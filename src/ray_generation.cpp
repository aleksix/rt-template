#include "ray_generation.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

//#define DEBUG

RayGenerationApp::RayGenerationApp(short width, short height) :
	width(width),
	height(height)
{
}

RayGenerationApp::~RayGenerationApp()
{
}

void RayGenerationApp::SetCamera(float3 position, float3 direction, float3 approx_up)
{
	camera.SetPosition(position);
	camera.SetDirection(direction);
	camera.SetUp(approx_up);
	camera.SetRenderTargetSize(width, height);
}

void RayGenerationApp::Clear()
{
	frame_buffer.resize(static_cast<size_t>(width * height));
}

void RayGenerationApp::DrawScene()
{
// Outer parallel works better on my machine, so it's kept here
#pragma omp parallel for
	for (short x = 0; x < width; ++x)
	{
// Inner parallel, just in case
//#pragma omp parallel for
		for (short y = 0; y < height; ++y)
		{
			Ray ray = camera.GetCameraRay(x, y);
			Payload payload = TraceRay(ray, raytracing_depth);
			SetPixel(x, y, payload.color);
		}
	}
}

int RayGenerationApp::Save(std::string filename) const
{
	int result = stbi_write_png(filename.c_str(), width, height, 3, frame_buffer.data(), width * 3);
// Custom debug macro, for better control
#ifdef DEBUG
	// Try to show the resulting image for manual checking
	if (result == 1)
	{
		system((std::string("start ") + filename).c_str());
	}
#endif
	return (1 - result);
}

Payload RayGenerationApp::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
	return Miss(ray);
}

Payload RayGenerationApp::Miss(const Ray& ray) const
{
	Payload out;
	float t = 0.5f * (ray.direction.y + 1.0f);
	out.color = float3(0.0f, 0.2f, 0.7f + 0.3f * t);
	return out;
}

void RayGenerationApp::SetPixel(unsigned short x, unsigned short y, float3 color)
{
	byte3 byte_color{ static_cast<uint8_t>(color.x * 255), 
		static_cast<uint8_t>(color.y * 255),
		static_cast<uint8_t>(color.z * 255) };
	frame_buffer[y * width + x] = byte_color;
}

void Camera::SetPosition(float3 position)
{
	this->position = position;
}

void Camera::SetDirection(float3 direction)
{
	this->direction = normalize(direction - this->position);
}

void Camera::SetUp(float3 approx_up)
{
	this->right = normalize(cross(this->direction, approx_up));
	this->up = normalize(cross(this->right, this->direction));
}

void Camera::SetRenderTargetSize(short width, short height)
{
	this->width = width;
	this->height = height;
}

Ray Camera::GetCameraRay(short x, short y) const
{
	float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
	float u = aspect_ratio * (((2.0f * (x + 0.5f)) / static_cast<float>(width)) - 1.0f);
	float v = ((2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(height)) - 1.0f);

	float3 ray_direction = direction + u * right - v * up;
	return Ray(position, ray_direction);
}

Ray Camera::GetCameraRay(short x, short y, float3 jitter) const
{
	return GetCameraRay(x, y);
}