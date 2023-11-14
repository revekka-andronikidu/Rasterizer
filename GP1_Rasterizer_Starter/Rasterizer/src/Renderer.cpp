//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

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

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	aspectRatio = m_Width / float(m_Height);
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
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
	m_Camera.Initialize(60.f, { 0.f, 0.f, -10.f });

	//RENDER LOGIC
	//RasterizationOnly(); 
	//ProjectionStage();
	BarycenticCoordinates();
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
		viewSpaceVertex.position = m_Camera.invViewMatrix.TransformPoint(vertex.position);

		Vertex projectedVertex{}; //projection space
		projectedVertex.position.x = viewSpaceVertex.position.x / viewSpaceVertex.position.z;
		projectedVertex.position.y = viewSpaceVertex.position.y / viewSpaceVertex.position.z;
		projectedVertex.position.z = viewSpaceVertex.position.z;

		projectedVertex.position.x = projectedVertex.position.x / (m_Camera.fov * aspectRatio);
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
	for (auto NDCvertex : vertices_ndc)
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
		{{-3.0f,-2.0f, 2.0f},{0,0,1}}
	};

	std::vector<Vertex> vertices{};
	VertexTransformationFunction(vertices_world, vertices);

	//𝑷 𝒊𝒏𝒔𝒊𝒅𝒆 𝑻𝒓𝒊𝒂𝒏𝒈𝒍𝒆=𝑾𝟎∗𝑽𝟎+𝑾𝟏∗𝑽𝟏+𝑾𝟐∗𝑽𝟐

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{ 0,0,0 };
			Vector3 pixel{ float(px) + 0.5f, float(py) + 0.5f , 0 }; // checking pixel from the center

			if (Utils::IsPointInTriangle(pixel, vertices))
			{
				float area = Vector2::Cross(Vector2((vertices[1].position.x - vertices[0].position.x), (vertices[1].position.y - vertices[0].position.y)), Vector2((vertices[2].position.x - vertices[0].position.x), (vertices[2].position.y - vertices[0].position.y)));

				float W0 = Vector2::Cross(Vector2((vertices[2].position.x - vertices[1].position.x), (vertices[2].position.y - vertices[1].position.y)), Vector2((pixel.x - vertices[1].position.x), (pixel.y - vertices[1].position.y))) / area;
				float W1 = Vector2::Cross(Vector2((vertices[0].position.x - vertices[2].position.x), (vertices[0].position.y - vertices[2].position.y)), Vector2((pixel.x - vertices[2].position.x), (pixel.y - vertices[2].position.y))) / area;
				float W2 = Vector2::Cross(Vector2((vertices[1].position.x - vertices[0].position.x), (vertices[1].position.y - vertices[0].position.y)), Vector2((pixel.x - vertices[0].position.x), (pixel.y - vertices[0].position.y))) / area;
				

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

}

void Renderer::BoundingBoxOptimization()
{

}
