#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : require

vec3 sunDirection = vec3(8000.0f, 10000.0f, 4000.0f); // Direction of the sun
float sunIntensity = 20.0f; // Intensity of the sun
vec3 sunColor = vec3(1.0f, 1.0f, 0.7f); // Color of the sun
vec3 horizonColor = vec3(0.5f, 0.5f, 0.8f); // Color of the sky at the horizon
vec3 zenithColor = vec3(0.0f, 0.0f, 0.8f); // Color of the sky at the zenith (directly above)

struct Payload
{
    vec4 color;
    bool isShadowed;
	int  depth;
};

layout(location = 0) rayPayloadInEXT Payload payload;

layout(binding = 0, set = 0) uniform accelerationStructureEXT as;
layout(binding = 2, set = 0) uniform UniformBuffer
{
    mat4 view_inv;
    mat4 proj_inv;
    float time;
} uniform_buffer;
layout(binding = 3, set = 0, scalar) buffer vertices_addresses_ { uint64_t address[]; } vertices_addresses;
layout(binding = 4, set = 0) uniform sampler2D tex_sampler;

struct Vertex
{
	vec3 pos; 
	vec2 tex_coord; 
};

layout(buffer_reference, scalar) buffer vertices_data { Vertex data[]; };

hitAttributeEXT vec3 attribs;

void main() 
{
	uint64_t address = vertices_addresses.address[gl_InstanceCustomIndexEXT];
	vertices_data vertices = vertices_data(address);
	
	const int index[2][3] = { {0, 1, 2} , {2, 3, 0} };
	
	Vertex v0 = vertices.data[gl_PrimitiveID / 2 * 4 + index[gl_PrimitiveID % 2][0]];
	Vertex v1 = vertices.data[gl_PrimitiveID / 2 * 4 + index[gl_PrimitiveID % 2][1]];
	Vertex v2 = vertices.data[gl_PrimitiveID / 2 * 4 + index[gl_PrimitiveID % 2][2]];

	vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	vec3 pos = vec3(v0.pos * bary.x + v1.pos * bary.y + v2.pos * bary.z);

	vec3 edge1 = v1.pos - v0.pos;
	vec3 edge2 = v2.pos - v0.pos;
	vec3 normal = normalize(cross(edge1, edge2));

	vec2 tex_coord = vec2(v0.tex_coord * bary.x + v1.tex_coord * bary.y + v2.tex_coord * bary.z);
	vec3 color = texture(tex_sampler, tex_coord).xyz;

	if (tex_coord.x >= 0.00347222225f  && tex_coord.x <= 0.0590277798f && tex_coord.y >= 0.815972209f && tex_coord.y <= 0.871527791f)	// beautifull check if water
	{
		if( payload.depth < 8)
		{
			vec3 rand_offset = vec3(0.0f);
			if(normal.y != 0)
			{
				float waveSpeed = 1.2; 
				float waveAmplitude = 0.0025; 
            
				// Define wave directions
				vec3 waveDir1 = normalize(vec3(1.0, 0.5, 0.3));
				vec3 waveDir2 = normalize(vec3(0.3, 0.7, -0.4));
				vec3 waveDir3 = normalize(vec3(-0.6, 0.4, 0.2));

				// Define frequencies and amplitudes for each wave
				float frequency1 = 2.0;
				float frequency2 = 3.0;
				float frequency3 = 1.5;
    
				float amplitude1 = waveAmplitude;
				float amplitude2 = waveAmplitude * 0.7;
				float amplitude3 = waveAmplitude * 0.9;

				// Calculate wave contributions
				float wave1 = amplitude1 * sin(dot(waveDir1, pos) * frequency1 + uniform_buffer.time * waveSpeed);
				float wave2 = amplitude2 * sin(dot(waveDir2, pos) * frequency2 + uniform_buffer.time * (waveSpeed * 0.8));
				float wave3 = amplitude3 * sin(dot(waveDir3, pos) * frequency3 + uniform_buffer.time * (waveSpeed * 1.2));

				rand_offset = vec3(wave1, wave2, wave3);
			}
			
			const float refraction_index_air = 1.0;
			const float refraction_index_water = 1.33;
			bool invert_refraction = false;
			float cos_theta  = dot(normalize(-gl_WorldRayDirectionEXT), normal);

			if (cos_theta < 0.0f)
			{
				normal = -normal;
				cos_theta  = -cos_theta;
				invert_refraction = true;
			}

			float r0 = (refraction_index_air - refraction_index_water) / (refraction_index_air + refraction_index_water);
			r0 = r0 * r0;
			float schlick_coef = r0 + (1.0f - r0) * pow(1.0f - cos_theta, 5.0f);

			payload.depth++;

			//reflection
			vec3 ro = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + 0.001 * normal;
			vec3 rd = reflect(normalize(gl_WorldRayDirectionEXT), normal + rand_offset);
			traceRayEXT(
			as,
			gl_RayFlagsOpaqueEXT,
			0xff,
			0, 0, 0,
			ro, 0.0001, rd, 10000.0,
			0
			);
			vec4 reflection_color = payload.color * 0.8;

			//refraction
			float eta = invert_refraction ? refraction_index_water / refraction_index_air : refraction_index_air / refraction_index_water;
			ro = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + 0.001 * -normal;
			rd = refract(normalize(gl_WorldRayDirectionEXT), normal + rand_offset, eta);
			if( rd != vec3(0.0f) )
				traceRayEXT(
				as,
				gl_RayFlagsOpaqueEXT,
				0xff,
				0, 0, 0,
				ro, 0.0001, rd, 10000.0,
				0
				);
			else
				schlick_coef = 1.0f;
			vec4 refraction_color = payload.color * 0.8;

			payload.depth--;

			payload.color = refraction_color * (1 - schlick_coef) + reflection_color * schlick_coef;
		}
		else
			payload.color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else
	{
		//Lighting
		vec3 light_dir = normalize(sunDirection);
		float ambient = 0.35f;
		float diffuse = max(dot(normal, light_dir), 0.0);

		float specular = 0.0f;
		float attenuation = 1;
		if(dot(normal, light_dir) > 0)
		{
			vec3  ro = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
			vec3  rd = light_dir;
			payload.isShadowed = true;
			traceRayEXT(
			as,
			gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
			0xff,
			0, 0, 0,
			ro, 0.001, rd, 10000.0,
			0
			);

			if(payload.isShadowed)
			{
				attenuation = 0.3;
			}
	//		else if (tex_coord.x > 0.00347222225f  && tex_coord.x < 0.0590277798f && tex_coord.y >0.815972209f && tex_coord.y < 0.871527791f)	// beautifull check if water
	//		{
	//			const float kPi = 3.14159265;
	//			const float kShininess = 256.0;
	//			const float kEnergyConservation = (2.0 + kShininess) / (2.0 * kPi);
	//			vec3 V = normalize(-gl_WorldRayDirectionEXT);
	//			vec3 R = reflect(-light_dir, normal);
	//			specular = kEnergyConservation * pow(max(dot(V, R), 0.0), kShininess);
	//		}
		}
		payload.color = vec4(color * attenuation * (ambient + diffuse) + vec3(1.0f) * specular, 0.0f);
	}
		if(gl_WorldRayOriginEXT.y < 129.0f)	//another ugly check, this time for being underwater
		{
			//payload.color = payload.color * clamp( exp(-0.2f * length(gl_WorldRayDirectionEXT * gl_HitTEXT)), 0.0, 1.0) / 0.3;
			//payload.color = payload.color * vec4(exp(-vec3(0.20, 0.10, 0.02) * (abs(gl_HitTEXT) * length(gl_WorldRayDirectionEXT))), 0.0f);
			//payload.color = payload.color * vec4(exp(-vec3(0.20, 0.14, 0.08) * (abs(gl_HitTEXT) * length(gl_WorldRayDirectionEXT))), 0.0f);

			payload.color = payload.color * vec4(exp(-vec3(0.18, 0.12, 0.06) * (abs(gl_HitTEXT) * length(gl_WorldRayDirectionEXT))), 0.0f);
		}
}
