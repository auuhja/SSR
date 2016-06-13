##GL_VERTEX_SHADER
#version 330

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texCoords;

out vec2 texCoords;

void main()
{
	texCoords = in_texCoords;
	gl_Position = vec4(in_position, 1.f);
}



##GL_FRAGMENT_SHADER
#version 330

in vec2 texCoords;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D colorTexture;

uniform sampler2D depthTexture;
uniform sampler2D backfaceDepthTexture;

uniform vec2 screenDim;

uniform mat4 proj;		// eye space to screen coordinates (NOT NDC)
uniform mat4 invProj;	// NDC to eye space

uniform vec2 clippingPlanes;

layout (location = 0) out vec4 fragColor;


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
	float depthFrontFace = linear01(texture2D(depthTexture, uv).x, nearPlane, farPlane) * farPlane;
	float depthBackFace = texture2D(backfaceDepthTexture, uv).x * farPlane; // why not linearize?

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
						 float iterations,
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

		hitPixel = permute ? pqk.yx : pqk.xy;
		hitPixel *= invScreenDim;

		intersect = rayIntersectsDepthBuffer(zA, zB, hitPixel, nearPlane, farPlane, pixelZSize);
	}

	// TODO: binary search refinement

	Q0.xy += dQ.xy * i;
	Q0.z = pqk.z;
	hitPoint = Q0 / pqk.w;
	iterationsNeeded = i;

	return intersect;
}


void main()
{
	vec3 color = texture2D(colorTexture, texCoords).rgb;
	vec3 position = texture2D(positionTexture, texCoords).xyz;
	vec3 normal = normalize(texture2D(normalTexture, texCoords).xyz);
	float shininess = texture2D(colorTexture, texCoords).a;
	
	float depth = linear01(texture2D(depthTexture, texCoords).x, clippingPlanes.x, clippingPlanes.y);
	fragColor = vec4(depth);

	//vec3 v = normalize(position);
	//vec3 rayDirection = reflect(v, normal);
	//vec3 rayOrigin = position;

	//float maxRayTraceDistance = 100.0;
	//float stride = 1.0;
	//float strideZCutoff = 100.0;
	//float jitter = 1.0;
	//float pixelZSize = 0.1;
	//float iterations = 60.0;

	

	//vec2 hitPixel;
	//vec3 hitPoint;
	//float iterationsNeeded;

	//bool result = traceScreenSpaceRay(rayOrigin, rayDirection, maxRayTraceDistance, 
	//					 stride, strideZCutoff, jitter, pixelZSize,
	//					 iterations,
	//					 hitPixel, hitPoint, iterationsNeeded);

	//if (result)
	//{
	//	vec2 tex = vec2(hitPixel.x / screenDim.x, (hitPixel.y / screenDim.y));
	//	//fragColor = texture2D(colorTexture, tex);
	//	fragColor = texture2D(colorTexture, hitPixel);
		
	//}
	//else
	//{
	//	fragColor = texture2D(colorTexture, texCoords);
	//	//fragColor = vec4(0.2);
	//}
	
	gl_FragDepth = texture2D(depthTexture, texCoords).x;
}


