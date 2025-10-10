#version 460
#extension GL_EXT_ray_tracing : enable

struct Payload
{
    vec4 color;
    bool isShadowed;
	int  depth;
};

layout(location = 0) rayPayloadInEXT Payload payload;

vec3 sunDirection = vec3(8000.0f, 10000.0f, 4000.0f); // Direction of the sun
float sunIntensity = 20.0f; // Intensity of the sun
vec3 sunColor = vec3(1.0f, 1.0f, 0.7f); // Color of the sun
vec3 horizonColor = vec3(0.5f, 0.5f, 0.8f); // Color of the sky at the horizon
vec3 zenithColor = vec3(0.0f, 0.0f, 0.8f); // Color of the sky at the zenith (directly above)

// Utility function to compute the smoothstep falloff
float smoothFalloff(float a, float b, float t) {
    return smoothstep(a, b, t);
}
//0.54f, 0.81f, 0.94f
void main() 
{
    // Get the ray direction from built-in variables
    vec3 rayDir = normalize(gl_WorldRayDirectionEXT); // Use the ray direction in world space
    
    // Determine the angle between the ray and the sun direction
    float sunDot = dot(rayDir, normalize(sunDirection));
    
    // Adjust these values to make the sun appear larger
    float sunSize = 0.988; // Decrease this to make the sun larger
    float sunEdgeSoftness = 0.044; // Increase this to make the sun edge softer
    
    // Compute sun disk: larger solid angle for the sun (smooth circular gradient)
    float sunDisk = smoothFalloff(sunSize, sunSize + sunEdgeSoftness, sunDot);
    
    // Base sky color: Simple linear interpolation between horizon and zenith color
    //float t = max(rayDir.y, 0.0); // Higher y value means closer to the zenith
    float t = (1 - cos(rayDir.y * rayDir.y * 3.14)) / 2;
    vec3 skyColor = mix(horizonColor, zenithColor, t);
    
    // Add sun contribution
    vec3 finalColor = skyColor + sunDisk * sunColor * sunIntensity;
    
    // Set the payload (result of this miss shader)
    payload.color = vec4(finalColor, 1.0);
    payload.isShadowed = false;

    //FAST FIX FOR BEING STUPID IDIOT, properly detecting when underwater+proper beer law should fix
    //now if under water ray miss without hitting anything (no edge of world) lets consider it should be black
    if(gl_WorldRayOriginEXT.y < 129.0f)	//another ugly check, this time for being underwater
	{
		payload.color = vec4(0.0f);
	}
}
