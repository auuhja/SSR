##GL_VERTEX_SHADER
#version 330

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texCoords;

out vec2 texCoords;

void main()
{
	texCoords = in_texCoords;
	gl_Position = vec4(in_position, 1.0);
}



##GL_FRAGMENT_SHADER
#version 330

in vec2 texCoords;

uniform sampler2D positionTexture; // current frame
uniform sampler2D normalTexture; // current frame

uniform sampler2D lastFrameColorTexture;
uniform sampler2D shininessTexture;

uniform sampler2D depthTexture;
uniform sampler2D backfaceDepthTexture;

uniform vec2 screenDim;

uniform mat4 proj;		// eye space to screen coordinates (NOT NDC)
uniform mat4 toPrevFramePos; // pixel pos from last frame

uniform vec2 clippingPlanes;

layout (location = 0) out vec4 out_reflectedColor;


void swap(inout float a, inout float b) 
{
     float temp = a;
     a = b;
     b = temp;
}

float distanceSquared(vec2 a, vec2 b) {
    a -= b;
    return dot(a, a);
}

float linear01(float depthValue, float n, float f)
{
	return (2.0 * n) / (f + n - depthValue * (f - n));
}

bool rayIntersectsDepthBuffer(float zA, float zB, vec2 uv, float nearPlane, float farPlane, float pixelZSize)
{
	float depthFrontFace = linear01(texture2D(depthTexture, uv).x, nearPlane, farPlane) * -farPlane;
	float depthBackFace = texture2D(backfaceDepthTexture, uv).x * -farPlane; // why not linearize?

	return zB <= depthFrontFace && zA >= depthBackFace - pixelZSize;
}

/*
	rayOrigin in viewspace
	rayDirection in viewspace
	maxRayDistance in viewspace
	stride - distance per step (in screenspace)
	strideZCutoff - from there on stride 1.0
	jitter
	pixelZSize - depth of pixel
*/

bool traceScreenSpaceRay(vec3 rayOrigin, vec3 rayDirection, float maxRayDistance, 
						 float stride, float strideZCutoff, float jitter, float pixelZSize,
						 float iterations, float binarySearchIterations,
						 out vec2 hitPixel, out vec3 hitPoint, out float iterationsNeeded)
{
	float nearPlane = clippingPlanes.x;
	float farPlane = clippingPlanes.y;

	float rayLength = 
		((rayOrigin.z + rayDirection.z * maxRayDistance) > -nearPlane) 
			? (-nearPlane - rayOrigin.z) / rayDirection.z 
			: maxRayDistance;

	vec3 rayEnd = rayOrigin + rayDirection * rayLength;

	// project into homogeneous clip space
	vec4 H0 = proj * vec4(rayOrigin, 1.0);
	vec4 H1 = proj * vec4(rayEnd, 1.0);

	float k0 = 1.0 / H0.w, k1 = 1.0 / H1.w;

	// interpolated homogeneous version of camera space points
	vec3 Q0 = rayOrigin * k0;
	vec3 Q1 = rayEnd * k1;

	// screen space endpoints
	vec2 P0 = H0.xy * k0;
	vec2 P1 = H1.xy * k1;

	// avoid degenerate lines
	P1 += (distanceSquared(P0, P1) < 0.0001) ? 0.01 : 0.0;

	vec2 delta = P1 - P0;

	bool permute = false;
	if (abs(delta.x) < abs(delta.y))
	{
		permute = true;
		delta = delta.yx;
		P0 = P0.yx;
		P1 = P1.yx;
	}

	float stepDir = sign(delta.x);
	float invdx = stepDir / delta.x;

	// derivatives of Q and k
	vec3 dQ = (Q1 - Q0) * invdx;
	float dk = (k1 - k0) * invdx;
	vec2 dP = vec2(stepDir, delta.y * invdx);

	float strideScaler = 1.0 - min(1.0, -rayOrigin.z / strideZCutoff);
	float pixelStride = 1.0 + strideScaler * stride;

	// scale derivatives by stride
	dP *= pixelStride; dQ *= pixelStride; dk *= pixelStride;
	P0 += dP * jitter; Q0 += dQ * jitter; k0 += dk * jitter;

	float i, zA = 0.0, zB = 0.0;

	// start values and derivatives packed together -> only one operation needed to increase
	vec4 pqk = vec4(P0, Q0.z, k0);
	vec4 dPQK = vec4(dP, dQ.z, dk);

	bool intersect = false;

	vec2 invScreenDim = vec2(1.0 / screenDim.x, 1.0 / screenDim.y);

	for (i = 0.0; i < iterations && !intersect; i += 1.0)
	{
		pqk += dPQK;

		zA = zB;
		// one half pixel into the future
		zB = (dPQK.z * 0.5 + pqk.z) / (dPQK.w * 0.5 + pqk.w);

		// swap if needed
		if (zB > zA)
		{
			swap(zB, zA);
		}

		// zA > zB

		hitPixel = permute ? pqk.yx : pqk.xy; // hitPixel = P
		hitPixel *= invScreenDim;

		intersect = rayIntersectsDepthBuffer(zA, zB, hitPixel, nearPlane, farPlane, pixelZSize);
	}

	// binary search refinement
	if( pixelStride > 1.0 && intersect)
	{
		pqk -= dPQK;
		dPQK /= pixelStride;
			    	
		float originalStride = pixelStride * 0.5;
		float stride = originalStride;
	        		
		zA = pqk.z / pqk.w;
		zB = zA;
	        		
		for( float j = 0; j < binarySearchIterations; j += 1.0)
		{
			pqk += dPQK * stride;
				    	
			zA = zB;
			zB = (dPQK.z * -0.5 + pqk.z) / (dPQK.w * -0.5 + pqk.w);
			if (zB > zA)
			{
				swap(zB, zA);
			}
				    	
			hitPixel = permute ? pqk.yx : pqk.xy;
			hitPixel *= invScreenDim;
				        
			originalStride *= 0.5;
			stride = rayIntersectsDepthBuffer( zA, zB, hitPixel, nearPlane, farPlane, pixelZSize) ? -originalStride : originalStride;
		}
	}

	Q0.xy += dQ.xy * i;
	Q0.z = pqk.z;
	hitPoint = Q0 / pqk.w;
	iterationsNeeded = i;

	return intersect;
}

float calculateAlphaForIntersection(float iterationCount, float specularStrength, vec2 hitPixel, vec3 hitPoint, vec3 rayOrigin, vec3 rayDirection, float iterations,
	float screenEdgeFadeStart, float eyeFadeStart, float eyeFadeEnd, float maxRayDistance)
{
	float alpha = min( 1.0, specularStrength * 1.0);
				
	// Fade ray hits that approach the maximum iterations
	alpha *= 1.0 - (iterationCount / iterations);
				
	// Fade ray hits that approach the screen edge
	float screenFade = screenEdgeFadeStart;
	vec2 hitPixelNDC = (hitPixel * 2.0 - 1.0);
	float maxDimension = min( 1.0, max( abs( hitPixelNDC.x), abs( hitPixelNDC.y)));
	alpha *= 1.0 - (max( 0.0, maxDimension - screenFade) / (1.0 - screenFade));
				
	// Fade ray hits base on how much they face the camera
	if (eyeFadeStart > eyeFadeEnd)
	{
		swap(eyeFadeStart, eyeFadeEnd);
	}
				
	float eyeDirection = clamp( rayDirection.z, eyeFadeStart, eyeFadeEnd);
	alpha *= 1.0 - ((eyeDirection - eyeFadeStart) / (eyeFadeEnd - eyeFadeStart));
				
	// Fade ray hits based on distance from ray origin
	alpha *= 1.0 - clamp( distance( rayOrigin, hitPoint) / maxRayDistance, 0.0, 1.0);
				
	return alpha;
}


void main()
{
	vec3 position = texture2D(positionTexture, texCoords).xyz;
	vec3 normal = normalize(texture2D(normalTexture, texCoords).xyz);
	float shininess = texture2D(shininessTexture, texCoords).x;
	
	gl_FragDepth = texture2D(depthTexture, texCoords).x;
	out_reflectedColor = vec4(0.0, 0.0, 0.0, 0.0);
	
	vec3 viewDir = normalize(position);
	vec3 rayDirection = normalize(reflect(viewDir, normal));
	vec3 rayOrigin = position;

	// parameters for ray tracing
	float maxRayTraceDistance = 100;
	float stride = 10;
	float strideZCutoff = 1000;
	float pixelZSize = 1;
	float iterations = 60;
	float binarySearchIterations = 10;

	vec2 uv2 = texCoords * screenDim;
	float c = (uv2.x + uv2.y) * 0.25;
	float jitter = mod( c, 1.0);
	

	vec2 hitPixel;
	vec3 hitPoint;
	float iterationsNeeded;

	bool result = traceScreenSpaceRay(rayOrigin, rayDirection, maxRayTraceDistance, 
						 stride, strideZCutoff, jitter, pixelZSize,
						 iterations, binarySearchIterations,
						 hitPixel, hitPoint, iterationsNeeded);

	if (result)
	{
		float specularStrength = 0.8; // TODO: parameter
		float screenEdgeFadeStart = 0.75;
		float eyeFadeStart = -10;
		float eyeFadeEnd = 10;

		float alpha = calculateAlphaForIntersection(iterationsNeeded, specularStrength, hitPixel, hitPoint, rayOrigin, rayDirection, iterations,
			screenEdgeFadeStart, eyeFadeStart, eyeFadeEnd, maxRayTraceDistance);

		vec4 prevFramePos = toPrevFramePos * vec4(hitPoint, 1.0);
		prevFramePos.xyz = prevFramePos.xyz / prevFramePos.w;

		vec2 tex = prevFramePos.xy * 0.5 + vec2(0.5);

		out_reflectedColor = vec4(texture2D(lastFrameColorTexture, tex).rgb, alpha);
	}
}


