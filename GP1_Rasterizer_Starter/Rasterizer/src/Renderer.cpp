//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"
#include <iostream>

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	m_AspectRatio = m_Width / float(m_Height);
	
	
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);
	

	//RENDER LOGIC
	//RasterizationOnly(); 
	//ProjectionStage();
	//BarycenticCoordinates();
	DepthBuffer();
	//BoundingBoxOptimization();

	//for (int px{}; px < m_Width; ++px)
	//{
	//	for (int py{}; py < m_Height; ++py)
	//	{
	//		RasterizationOnly(px, py);
	//		/*float gradient = px / static_cast<float>(m_Width);
	//		gradient += py / static_cast<float>(m_Width);
	//		gradient /= 2.0f;
	//
	//		ColorRGB finalColor{ gradient, gradient, gradient };*/
	//
	//		//Update Color in Buffer
	//		/*finalColor.MaxToOne();
	//
	//		m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
	//			static_cast<uint8_t>(finalColor.r * 255),
	//			static_cast<uint8_t>(finalColor.g * 255),
	//			static_cast<uint8_t>(finalColor.b * 255));*/
	//	}
	//}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	
	
	vertices_out.clear();
	vertices_out.reserve(vertices_in.size());

	for (auto& vertex : vertices_in)
	{
		// from world space to camera (view) space by multiplying with inverse of camera matrix
		Vertex viewSpaceVertex{};  //view space 
		viewSpaceVertex.position = m_Camera.invViewMatrix.TransformPoint({ vertex.position, 1.f });

		Vertex projectedVertex{}; //projection space
		projectedVertex.position.x = viewSpaceVertex.position.x / viewSpaceVertex.position.z;
		projectedVertex.position.y = viewSpaceVertex.position.y / viewSpaceVertex.position.z;
		projectedVertex.position.z = viewSpaceVertex.position.z;

		projectedVertex.position.x = projectedVertex.position.x / (m_Camera.fov * m_AspectRatio);
		projectedVertex.position.y = projectedVertex.position.y / m_Camera.fov;

		Vertex screenSpaceVertex{}; //screen space

		screenSpaceVertex.position.x = ((projectedVertex.position.x + 1) / 2) * m_Width;
		screenSpaceVertex.position.y = ((1 - projectedVertex.position.y) / 2) * m_Height;
		screenSpaceVertex.position.z = projectedVertex.position.z;
		
		screenSpaceVertex.color = vertex.color;
		vertices_out.emplace_back(screenSpaceVertex);
	}

}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::RasterizationOnly()
{
	//Triangle - Vertices in NDC space (Normalized Device Coordinates)
	std::vector<Vertex> vertices_ndc
	{
		{{0.0f, 0.5f, 1.0f}},
		{{0.5f,-0.5f, 1.0f}},
		{{-0.5f, -0.5f, 1.0f}},
	};
	
	std::vector<Vertex> screenSpaceVertices{};
	
	//convert from NDC to screen space
	// ScreenSpaceVertexX = ((NDCvertexX+1)/2) * ScreenWidth
	// ScreenSpaceVertexY = ((1 -NDCvertexY)/2) * ScreenHeight
	for (auto &NDCvertex : vertices_ndc)
	{
		float ScreenSpaceVertexX = ((NDCvertex.position.x + 1) / 2) * m_Width;
		float ScreenSpaceVertexY = ((1 - NDCvertex.position.y) / 2) * m_Height;
		screenSpaceVertices.push_back({ { ScreenSpaceVertexX, ScreenSpaceVertexY, NDCvertex.position.z }, NDCvertex.color});
 	}

		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				ColorRGB finalColor{ 0,0,0 };
				Vector3 pixel{ float(px) + 0.5f, float(py) + 0.5f , 0}; // checking pixel from the center

				if (Utils::IsPointInTriangle(pixel, screenSpaceVertices))
				{
					finalColor = ColorRGB{ 1,1,1 };
				}

				//Update Color in Buffer
				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}	
}

void Renderer::ProjectionStage()
{
		std::vector<Vertex> vertices_world
	{ 
		{{0.0f, 2.0f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}},
		{{-1.0f, 0.0f, 0.0f}}
	};


	std::vector<Vertex> vertices{};
	VertexTransformationFunction(vertices_world, vertices);

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{ 0,0,0 };
			Vector3 pixel{ float(px) + 0.5f, float(py) + 0.5f , 0 }; // checking pixel from the center

			if (Utils::IsPointInTriangle(pixel, vertices))
			{
				finalColor = ColorRGB{ 1,1,1 };
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::BarycenticCoordinates()
{
	std::vector<Vertex> vertices_world
	{
		{{0.0f, 4.0f, 2.0f}, {1,0,0}},
		{{3.0f,-2.0f, 2.0f}, {0,1,0}},
		{{-3.0f,-2.0f, 2.0f},{0,0,1}},
	};

	std::vector<Vertex> vertices{};
	VertexTransformationFunction(vertices_world, vertices);

	//𝑷 𝒊𝒏𝒔𝒊𝒅𝒆 𝑻𝒓𝒊𝒂𝒏𝒈𝒍𝒆=𝑾𝟎∗𝑽𝟎+𝑾𝟏∗𝑽𝟏+𝑾𝟐∗𝑽𝟐

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{ 0,0,0 };
			Vector3 point{ float(px) + 0.5f, float(py) + 0.5f , 0 }; // checking pixel from the center
			Vector2 pixel{ float(px) + 0.5f, float(py) + 0.5f};

			if (Utils::IsPointInTriangle(point, vertices))
			{
				const Vector2 v0{ vertices[0].position.x, vertices[0].position.y };
				const Vector2 v1{ vertices[1].position.x, vertices[1].position.y };
				const Vector2 v2{ vertices[2].position.x, vertices[2].position.y };

				float area = Vector2::Cross(v1 - v0, v2 - v0);

				float W0 = Vector2::Cross(v2 - v1, pixel - v1) / area;
				float W1 = Vector2::Cross(v0 - v2, pixel - v2) / area;
				float W2 = Vector2::Cross(v1 - v0, pixel - v0) / area;

				//Interpolated Color 𝑽𝒆𝒓𝒕𝒆𝒙𝑪𝒐𝒍𝒐𝒓𝟎∗𝑾𝟎 + 𝑽𝒆𝒓𝒕𝒆𝒙𝑪𝒐𝒍𝒐𝒓𝟏∗𝑾𝟏 + 𝑽𝒆𝒓𝒕𝒆𝒙𝑪𝒐𝒍𝒐𝒓𝟐∗𝑾𝟐		
				finalColor = (vertices[0].color * W0) + (vertices[1].color * W1) + (vertices[2].color * W2);
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::DepthBuffer()
{	
	std::vector<Vertex> vertices_world
	{
		//triangle1
		{{0.0f, 2.0f, 0.0f}, {1, 0, 0}},
		{{1.5f, -1.0f, 0.0f}, {1, 0, 0}},
		{{-1.5f, -1.0f, 0.0f}, {1, 0, 0}},

		////triangle 2
		{{0.0f, 4.0f, 2.0f}, {1, 0, 0}},
		{{3.0f, -2.0f, 2.0f}, {0, 1, 0}},
		{{-3.0f, -2.0f, 2.0f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices{};
	VertexTransformationFunction(vertices_world, vertices);

	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	
	
	//check each triangle
		for (int index{ 0 }; index < vertices.size(); index +=3)
	{
		std::vector<Vertex> triangle
			{
				vertices[index + 0],
				vertices[index + 1],
				vertices[index + 2],
			};

		const Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
		const Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
		const Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

		float area = Vector2::Cross(v1 - v0, v2 - v0);

		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				ColorRGB finalColor{ 0,0,0 };

				Vector2 pixel{ float(px) + 0.5f, float(py) + 0.5f}; // checking pixel from the center
				Vector3 point{ float(px) + 0.5f, float(py) + 0.5f , 0 };
				const int pixelIdx{ px + py * m_Width };

				if (Utils::IsPointInTriangle(point, triangle))
				{
					float W0 = Vector2::Cross(v2 - v1, pixel - v1) / area;
					float W1 = Vector2::Cross(v0 - v2, pixel - v2) / area;
					float W2 = Vector2::Cross(v1 - v0, pixel - v0) / area;


					const float depth0{ triangle[0].position.z };
					const float depth1{ triangle[1].position.z };
					const float depth2{ triangle[2].position.z };

					const float interpolatedDepth = depth0 * W0 + depth1 * W1 + depth2 * W2;
					//
					//const float interpolatedDepth{ 1.f / (W0 * (1.f / depth0) + W1 * (1.f / depth1) + W2 * (1.f / depth2)) };
					
					float depth = m_pDepthBufferPixels[pixelIdx];

					//if (depth < interpolatedDepth || interpolatedDepth < 0.f || interpolatedDepth > 1.f) continue;
					
					if (depth > interpolatedDepth)
					{
						m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;

						finalColor = (triangle[0].color * W0) + (triangle[1].color * W1) + (triangle[2].color * W2);
						// Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
	
}

void Renderer::BoundingBoxOptimization()
{
	std::vector<Vertex> vertices_world
	{
		//triangle1
		{{0.0f, 2.0f, 0.0f}, {1, 0, 0}},
		{{1.5f, -1.0f, 0.0f}, {1, 0, 0}},
		{{-1.5f, -1.0f, 0.0f}, {1, 0, 0}},

		////triangle 2
		{{0.0f, 4.0f, 2.0f}, {1, 0, 0}},
		{{3.0f, -2.0f, 2.0f}, {0, 1, 0}},
		{{-3.0f, -2.0f, 2.0f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices{};
	VertexTransformationFunction(vertices_world, vertices);

	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));



	//check each triangle
	for (int index{ 0 }; index < vertices.size(); index += 3)
	{
		std::vector<Vertex> triangle
		{
			vertices[index + 0],
			vertices[index + 1],
			vertices[index + 2],
		};

		const Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
		const Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
		const Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

		float area = Vector2::Cross(v1 - v0, v2 - v0);

		//bounding box
		int minX = std::min({ triangle[0].position.x, triangle[1].position.x, triangle[2].position.x });
		int maxX = std::max({ triangle[0].position.x, triangle[1].position.x, triangle[2].position.x });;
		int minY = std::min({ triangle[0].position.y, triangle[1].position.y, triangle[2].position.y });;
		int maxY = std::max({ triangle[0].position.y, triangle[1].position.y, triangle[2].position.y });;

		if (minX < 0) minX = 0;
		if (maxX > m_Width) maxX = m_Width;

		if (minY < 0) minY = 0;
		if (maxY > m_Height) maxY = m_Height;

		for (int px{minX}; px < maxX; ++px)
		{
			for (int py{minY}; py < maxY; ++py)
			{
				ColorRGB finalColor{ 0,0,0 };

				Vector2 pixel{ float(px) + 0.5f, float(py) + 0.5f }; // checking pixel from the center
				Vector3 point{ float(px) + 0.5f, float(py) + 0.5f , 0 };
				const int pixelIdx{ px + py * m_Width };

				if (Utils::IsPointInTriangle(point, triangle))
				{
					float W0 = Vector2::Cross(v2 - v1, pixel - v1) / area;
					float W1 = Vector2::Cross(v0 - v2, pixel - v2) / area;
					float W2 = Vector2::Cross(v1 - v0, pixel - v0) / area;


					const float depth0{ triangle[0].position.z };
					const float depth1{ triangle[1].position.z };
					const float depth2{ triangle[2].position.z };

					const float interpolatedDepth = depth0 * W0 + depth1 * W1 + depth2 * W2;
					//
					//const float interpolatedDepth{ 1.f / (W0 * (1.f / depth0) + W1 * (1.f / depth1) + W2 * (1.f / depth2)) };

					float depth = m_pDepthBufferPixels[pixelIdx];

					//if (depth < interpolatedDepth || interpolatedDepth < 0.f || interpolatedDepth > 1.f) continue;

					if (depth > interpolatedDepth)
					{
						m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;

						finalColor = (triangle[0].color * W0) + (triangle[1].color * W1) + (triangle[2].color * W2);
						// Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}

}

