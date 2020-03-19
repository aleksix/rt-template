#include "refraction.h"

Refraction::Refraction(short width, short height) :Reflection(width, height)
{
    //raytracing_depth = 3;
}

Refraction::~Refraction()
{
}

Payload Refraction::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int max_raytrace_depth) const
{
    if (max_raytrace_depth == 0)
        return Miss(ray);

    if (triangle == nullptr)
        return Miss(ray);

    Payload out;

    out.color = triangle->emissive_color;

    float3 X = ray.position + ray.direction * data.t;

    float3 N = triangle->GetNormal(data.baricentric);

    if (triangle->reflectiveness)
    {
        float3 reflection_direction = ray.direction - 2.0f * dot(N, ray.direction) * N;
        Ray reflection_ray(X, reflection_direction);
        return TraceRay(reflection_ray, max_raytrace_depth - 1);
    }

    else if (triangle->reflectiveness_and_transparency)
    {
        float kr;
        float cos_input = std::max(-1.f, std::min(1.f, dot(ray.direction, N)));
        
        float eta_input = 1.f;
        float eta_output = triangle->ior;
        if(cos_input > 0.0f)
        {
            std::swap(eta_input, eta_output);
        }

        float sin_output = eta_input / eta_output * sqrtf(std::max(0.0f, 1 - cos_input * cos_input));

        if (sin_output >= 1.0f)
        {
            kr = 1.0f;
        }
        else
        {
            float cos_output = sqrtf(std::max(0.0f, 1 - sin_output * sin_output));
            cos_input = fabs(cos_input);
            float Rs = (eta_output * cos_input - eta_input * cos_output) / (eta_output * cos_input + eta_input * cos_output);
            float Rp = (eta_input * cos_input - eta_output * cos_output) / (eta_input * cos_input + eta_output * cos_output);
            kr = (Rs * Rs + Rp * Rp) / 2.0f;
        }

        bool outside = (dot(ray.direction, N) < 0);
        float3 bias = 0.001f * N;

        Payload refraction;

        if (kr < 1.0f)
        {
            float cos_input = std::max(-1.f, std::min(1.f, dot(ray.direction, N)));
            float eta_input = 1.f;
            float eta_output = triangle->ior;
            if (cos_input > 0.f)
            {
                std::swap(eta_input, eta_output);
            }
            else
            {
                cos_input = -cos_input;
            }

            cos_input = fabs(cos_input);
            float eta = eta_input / eta_output;
            float k = 1.0f - eta * eta * (1.0f - cos_input * cos_input);
            float3 refraction_direction{ 0, 0, 0 };
            if (k >= 0)
            {
                refraction_direction = eta * ray.direction + (eta * cos_input - sqrtf(k)) * N;
            }

            Ray refraction_ray(outside ? X - bias : X + bias, refraction_direction);
            refraction = TraceRay(refraction_ray, max_raytrace_depth - 1);
        }

        float3 reflection_direction = ray.direction - 2.0f * dot(N, ray.direction) * N;
        Ray reflection_ray(outside ? X + bias : X - bias, reflection_direction);
        Payload reflection = TraceRay(reflection_ray, max_raytrace_depth - 1);

        Payload summary;
        summary.color = reflection.color * kr + refraction.color * (1.0f - kr);

        return summary;
    }

    // Diffuse and specular
    for (auto light : lights)
    {
        float to_light_distance = length(light->position - X);
        Ray to_light(X, light->position - X);

        float actual_t = TraceShadowRay(to_light, to_light_distance);
        if (fabs(actual_t - to_light_distance) > 1e-6)
            continue;

        out.color += light->color * triangle->diffuse_color * std::max(0.0f, dot(N, to_light.direction));

        float3 reflection_direction = 2.0f * dot(N, to_light.direction) * N - to_light.direction;
        out.color += light->color * triangle->specular_color *
            powf(std::max(0.0f, dot(ray.direction, reflection_direction)), triangle->specular_exponent);
    }

    return out;
}
