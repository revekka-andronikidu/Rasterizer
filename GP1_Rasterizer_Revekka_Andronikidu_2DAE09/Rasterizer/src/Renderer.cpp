//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"
#include <iostream>
#include "BRDFs.h"
//my includes
#include <vector>
#include <ppl.h>

using namespace dae;
using namespace Utils;

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
	m_Camera.Initialize(45.f, { .0f, 5.f,-64.f });

	m_AspectRatio = m_Width / float(m_Height);
	
	m_pDiffuseTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pSpecularTexture = Texture::LoadFromFile("Resources/vehicle_specular.png");
	m_pGlossinessTexture = Texture::LoadFromFile("Resources/vehicle_gloss.png");
	m_pNormalTexture = Texture::LoadFromFile("Resources/vehicle_normal.png");
	

	//meshes
	//m_pMesh = new Mesh
	//{
	//	{
	//		Vertex{{ -3.0f,  3.0f, -2.0f},{1,1,1},{ 0.0f, 0.0f}},
	//		Vertex{ {  0.0f,  3.0f, -2.0f},{1,1,1},{ 0.5f, 0.0f} },
	//		Vertex{ {  3.0f,  3.0f, -2.0f},{1,1,1},{ 1.0f, 0.0f} },
	//		Vertex{ { -3.0f,  0.0f, -2.0f},{1,1,1},{ 0.0f, 0.5f} },
	//		Vertex{ {  0.0f,  0.0f, -2.0f},{1,1,1},{ 0.5f, 0.5f} },
	//		Vertex{ {  3.0f,  0.0f, -2.0f},{1,1,1},{ 1.0f, 0.5f} },
	//		Vertex{ { -3.0f, -3.0f, -2.0f},{1,1,1},{ 0.0f, 1.0f} },
	//		Vertex{ {  0.0f, -3.0f, -2.0f},{1,1,1},{ 0.5f, 1.0f} },
	//		Vertex{ {  3.0f, -3.0f, -2.0f},{1,1,1},{ 1.0f, 1.0f} },
	//	},
	//	{
	//		3,0,4,1,5,2,
	//		2,6,
	//		6,3,7,4,8,5
	//	},
	//	PrimitiveTopology::TriangleStrip,
	//};

	m_pMesh = new Mesh();
	Utils::ParseOBJ("Resources/vehicle.obj", m_pMesh->vertices, m_pMesh->indices);
	
	m_pMesh->Translate(0.f, 0.f, 10.f);
	m_pMesh->primitiveTopology = PrimitiveTopology::TriangleList;
	
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pDiffuseTexture;
	delete m_pSpecularTexture;
	delete m_pGlossinessTexture;
	delete m_pNormalTexture;
	delete m_pMesh;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	constexpr const float rotationSpeed{ 30.f };
	if (m_EnableRotating) { m_pMesh->RotateY(rotationSpeed * pTimer->GetElapsed()); }

	const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
	if (pKeyboardState[SDL_SCANCODE_F4])
	{
		if (!m_F4Held)
		{
			m_RenderMode = static_cast<RenderMode>((static_cast<int>(m_RenderMode) + 1) % (static_cast<int>(RenderMode::END)));


			std::cout << "[RENDERMODE] ";
			switch (m_RenderMode)
			{
			case dae::Renderer::RenderMode::Texture:
				std::cout << "Texture\n";
				break;
			case dae::Renderer::RenderMode::Buffer:
				std::cout << "Depth Buffer\n";
				break;
			}
		}
		m_F4Held = true;
	}
	else
		m_F4Held = false;

	if (pKeyboardState[SDL_SCANCODE_F7])
	{
		if (!m_F7Held)
		{
			m_ShadingMode = static_cast<ShadingMode>((static_cast<int>(m_ShadingMode) + 1) % (static_cast<int>(ShadingMode::END)));


			std::cout << "[SHADING MODE] ";
			switch (m_ShadingMode)
			{
			case dae::Renderer::ShadingMode::ObservedArea:
				std::cout << "Observed Area\n";
				break;
			case dae::Renderer::ShadingMode::Specular:
				std::cout << "Specular\n";
				break;
			case dae::Renderer::ShadingMode::Diffuse:
				std::cout << "Difuse\n";
				break;
			case dae::Renderer::ShadingMode::Combined:
				std::cout << "Combined\n";
				break;
			case ShadingMode::Ambient:
				std::cout << "Ambient\n";
				break;
			}
		}
		m_F7Held = true;
	}
	else
		m_F7Held = false;

	if (pKeyboardState[SDL_SCANCODE_F5])
	{
		if (!m_F5Held)
		{
			m_EnableRotating = !m_EnableRotating;
			std::cout << "[ROTATION] ";
			std::cout << (m_EnableRotating ? "Rotation enabled\n" : "Rotation disabled\n");
		}
		m_F5Held = true;
	}
	else m_F5Held = false;

	if (pKeyboardState[SDL_SCANCODE_F6])
	{
		if (!m_F6Held)
		{
			m_EnableNormalMap = !m_EnableNormalMap;
			std::cout << "[NORMAL MAP] ";
			std::cout << (m_EnableNormalMap ? "NormalMap enabled\n" : "NormalMap disabled\n");
		}
		m_F6Held = true;
	}
	else m_F6Held = false;

}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX); //reset depth buffer
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 0, 0, 0)); //clear background
	

	//RENDER LOGIC
	std::vector<Mesh> meshes_world;
	meshes_world.push_back(*m_pMesh);
	RenderMeshes(meshes_world);

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(Mesh& mesh)
{
	//WorldViewProjectionMatrix = WorldMatrix ∗ ViewMatrix ∗ ProjectionMatrix
	Matrix worldViewProjectionMatrix{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };
		
	mesh.vertices_out.clear();
	mesh.vertices_out.reserve(mesh.vertices.size());

		/*Week 1 & 2*/
		//for (auto& vertex : mesh.vertices)
		//{
		//	//from world space to camera(view) space by multiplying with inverse of camera matrix
		//	Vertex_Out viewSpaceVertex {};  //view space 
		//	viewSpaceVertex.position = m_Camera.invViewMatrix.TransformPoint({ vertex.position, 1.f });
		//
		//	Vertex_Out projectedVertex{}; //projection space
		//	projectedVertex.position.x = viewSpaceVertex.position.x / viewSpaceVertex.position.z;
		//	projectedVertex.position.y = viewSpaceVertex.position.y / viewSpaceVertex.position.z;
		//	projectedVertex.position.z = viewSpaceVertex.position.z;
		//
		//	projectedVertex.position.x = projectedVertex.position.x / (m_Camera.fov * m_AspectRatio);
		//	projectedVertex.position.y = projectedVertex.position.y / m_Camera.fov;
		//
		//	Vertex_Out screenSpaceVertex{}; //screen space
		//
		//	screenSpaceVertex.position.x = ((projectedVertex.position.x + 1) / 2) * m_Width;
		//	screenSpaceVertex.position.y = ((1 - projectedVertex.position.y) / 2) * m_Height;
		//	screenSpaceVertex.position.z = projectedVertex.position.z;
		//
		//	screenSpaceVertex.color = vertex.color;
		//	screenSpaceVertex.uv = vertex.uv;
		//	mesh.vertices_out.emplace_back(screenSpaceVertex);
		//}

		for (auto& vertex : mesh.vertices)
		{
			Vertex_Out vertex_out{ Vector4{}, vertex.color, vertex.uv, vertex.normal, vertex.tangent };

			vertex_out.position = worldViewProjectionMatrix.TransformPoint({ vertex.position, 1.0f });
			vertex_out.viewDirection = Vector3{ vertex_out.position.x, vertex_out.position.y, vertex_out.position.z }.Normalized();

			vertex_out.normal = mesh.worldMatrix.TransformVector(vertex.normal);
			vertex_out.tangent = mesh.worldMatrix.TransformVector(vertex.tangent);

			//perspective divide to put vertices in NDC
			const float invertedViewSpaceW{ 1 / vertex_out.position.w };
			vertex_out.position.x *= invertedViewSpaceW;
			vertex_out.position.y *= invertedViewSpaceW;
			vertex_out.position.z *= invertedViewSpaceW;

			vertex_out.position.x = vertex_out.position.x / m_AspectRatio;// / (m_Camera.fov * m_AspectRatio);
			vertex_out.position.y = vertex_out.position.y  ;// / m_Camera.fov;

			//emplace back because we made vOut just to store in this vector
			mesh.vertices_out.emplace_back(vertex_out);
		}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::RenderTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2) const
{
	// If a triangle has the same vertex twice, it means it has no surface and can't be rendered.
	if (v0 == v1 || v1 == v2 || v2 == v0)
	{
		return;
	}

	std::vector<Vertex_Out> triangle
	{
		v0,
		v1,
		v2,
	};

	const Vector2 V0{ v0.position.x, v0.position.y };
	const Vector2 V1{ v1.position.x, v1.position.y };
	const Vector2 V2{ v2.position.x, v2.position.y };

	//bounding box
	int minX = std::min({ v0.position.x, v1.position.x, v2.position.x });
	int maxX = std::max({ v0.position.x, v1.position.x, v2.position.x });;
	int minY = std::min({ v0.position.y, v1.position.y, v2.position.y });;
	int maxY = std::max({ v0.position.y, v1.position.y, v2.position.y });;

	// Add margin to prevent seethrough lines between quads
	const float margin{ 1.f };
	minX -= margin;
	minY -= margin;
	maxX += margin;
	maxY += margin;

	// Make sure the boundingbox is on the screen
	minX = Clamp(minX, 0, m_Width);
	minY = Clamp(minY, 0, m_Height);
	maxX = Clamp(maxX, 0, m_Width);
	maxY = Clamp(maxY, 0, m_Height);

	float area = Vector2::Cross(V1 - V0, V2 - V0);

	for (int px{ minX }; px < maxX; ++px)
	{
		for (int py{ minY }; py < maxY; ++py)
		{
			Vector2 pixel{ float(px) + 0.5f, float(py) + 0.5f }; // checking pixel from the center
			Vector3 point{ float(px) + 0.5f, float(py) + 0.5f , 0 };
			const int pixelIdx{ px + py * m_Width };

			//const bool isInTriangle = weight0 > 0 && weight1 > 0 && weight2 > 0;
			if (Utils::IsPointInTriangle(point, triangle))
			{
				float weight0 = Vector2::Cross(V2 - V1, pixel - V1) / area;
				float weight1 = Vector2::Cross(V0 - V2, pixel - V2) / area;
				float weight2 = Vector2::Cross(V1 - V0, pixel - V0) / area;

				const float depth0{ v0.position.z };
				const float depth1{ v1.position.z };
				const float depth2{ v2.position.z };

				//const float interpolatedZ{ 1.f / ((weight0 * (1.f / v0.position.z) + (weight1 * (1.f / v1.position.z)) + (weight2 * (1.f / v2.position.z)))) };
				const float interpolatedDepth = CalculateInterpolatedZ(triangle, weight0, weight1, weight2); //interpolated Z


				//makes sure that the object is not rendered if it is behind camera(otherwise mirrored)  (Frustum clipping)
				float depth = m_pDepthBufferPixels[pixelIdx];
				if (depth < interpolatedDepth || interpolatedDepth < 0.f || interpolatedDepth > 1.f) continue;
				m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;

				
				Vertex_Out pixelOut{};
				pixelOut.position = { pixel.x,pixel.y, interpolatedDepth,interpolatedDepth };
				pixelOut.uv = ((v0.uv / depth0) * weight0 + (v1.uv / depth1) * weight1 + (v2.uv / depth2) * weight2) * interpolatedDepth;
				pixelOut.normal = Vector3{ interpolatedDepth * (weight0 * v0.normal / v0.position.w + weight1 * v1.normal / v1.position.w + weight2 * v2.normal / v2.position.w) }.Normalized();
				pixelOut.tangent = Vector3{ interpolatedDepth * (weight0 * v0.tangent / v0.position.w + weight1 * v1.tangent /v1.position.w + weight2 * v2.tangent / v2.position.w) }.Normalized();
				pixelOut.viewDirection = Vector3{ interpolatedDepth * (weight0 * v0.viewDirection / v0.position.w + weight1 * v1.viewDirection / v1.position.w + weight2 * v2.viewDirection /v2.position.w) }.Normalized();

				PixelShading(pixelOut);
			}
		}
	}

}

void Renderer::NDCtoScreenSpace(Vertex_Out& v0, Vertex_Out& v1, Vertex_Out& v2)
{
	//NDC --> Screenspace
	v0.position.x = ((v0.position.x + 1.f) / 2.f) * m_Width;
	v0.position.y = ((1.f - v0.position.y) / 2.f) * m_Height;
	v1.position.x = ((v1.position.x + 1.f) / 2.f) * m_Width;
	v1.position.y = ((1.f - v1.position.y) / 2.f) * m_Height;
	v2.position.x = ((v2.position.x + 1.f) / 2.f) * m_Width;
	v2.position.y = ((1.f - v2.position.y) / 2.f) * m_Height;
}

void Renderer::RenderMeshes(std::vector<Mesh> meshes_world)
{
	for (auto& mesh : meshes_world)
	{
		VertexTransformationFunction(mesh);


		Vertex_Out v0, v1, v2;
		switch (mesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangleStrip:
		{
			for (int indicesIndex{}; indicesIndex < mesh.indices.size() - 2; indicesIndex++)
			{

				if (indicesIndex & 1)
				{
					v0 = mesh.vertices_out[mesh.indices[2 + indicesIndex]];
					v1 = mesh.vertices_out[mesh.indices[1 + indicesIndex]];
					v2 = mesh.vertices_out[mesh.indices[indicesIndex]];
				}
				else
				{
					v0 = mesh.vertices_out[mesh.indices[indicesIndex]];
					v1 = mesh.vertices_out[mesh.indices[1 + indicesIndex]];
					v2 = mesh.vertices_out[mesh.indices[2 + indicesIndex]];
					
				}			

				//clipping
				if (IsVertexInFrustrum(v0.position) && IsVertexInFrustrum(v1.position) && IsVertexInFrustrum(v2.position))
				{
					NDCtoScreenSpace(v0, v1, v2);
					RenderTriangle(v0, v1, v2);
				}
				/*else
				{
					Clipping(v0, v1, v2);
				}*/

			}
		}
		break;
		case PrimitiveTopology::TriangleList:
		{
			for (int indicesIndex{}; indicesIndex < mesh.indices.size(); indicesIndex += 3)
			{
				v0 = mesh.vertices_out[mesh.indices[indicesIndex]];
				v1 = mesh.vertices_out[mesh.indices[1 + indicesIndex]];
				v2 = mesh.vertices_out[mesh.indices[2 + indicesIndex]];

				//clipping
				if (IsVertexInFrustrum(v0.position) && IsVertexInFrustrum(v1.position) && IsVertexInFrustrum(v2.position))
				{
					NDCtoScreenSpace(v0, v1, v2);
					RenderTriangle(v0, v1, v2);
				}
			}
		}
		break;
		}

	}
}

void Renderer::PixelShading(const Vertex_Out& v) const
{
	Vector3 lightDirection = { .577f, -.577f, .577f };
	const float lightIntensity{ 7.f };
	ColorRGB finalColor{ 0, 0, 0 };
	Vector3 normal{ v.normal };


	if (m_EnableNormalMap)
	{
		Vector3 binormal = Vector3::Cross(v.normal, v.tangent);
		Matrix tangentSpaceAxis = Matrix{ v.tangent, binormal, v.normal, Vector3::Zero };

		const ColorRGB normalSampleVecCol{ (2 * m_pNormalTexture->Sample(v.uv)) - ColorRGB{1,1,1} };
		const Vector3 normalSampleVec{ normalSampleVecCol.r,normalSampleVecCol.g,normalSampleVecCol.b };
		normal = tangentSpaceAxis.TransformVector(normalSampleVec);
	}

	
	const float observedArea{Vector3::DotClamp(normal.Normalized(), -lightDirection)};
	const ColorRGB lambert{ BRDF::Lambert(1.0f, m_pDiffuseTexture->Sample(v.uv))};
	const float specularVal{ m_SpecularShininess * m_pGlossinessTexture->Sample(v.uv).r };
	const ColorRGB specular{ m_pSpecularTexture->Sample(v.uv) * BRDF::Phong(1.0f, specularVal, -lightDirection, v.viewDirection, normal) };

	switch (m_RenderMode)
	{
	case RenderMode::Texture:
	
		switch (m_ShadingMode)
		{
		case ShadingMode::ObservedArea:
			finalColor = ColorRGB{ observedArea, observedArea, observedArea };
			break;
		case ShadingMode::Diffuse:

			finalColor = lightIntensity * observedArea * lambert;

			break;
		case ShadingMode::Specular:
			finalColor = specular * observedArea;
			break;
		case ShadingMode::Combined:
			finalColor = lightIntensity * observedArea * lambert + specular;
			break;
		case ShadingMode::Ambient:
			finalColor = ColorRGB{ 0.3f, 0.3f, 0.3f };
			break;
		}
	
	break;
	case RenderMode::Buffer:
	
		//finalColor = (v0.color * weight0) + (v1.color * weight1) + (v2.color * weight2); //just color no depth visualization
		const float depthCol{ Remap(v.position.w,0.990f,1.f) };
		finalColor = { depthCol,depthCol,depthCol };
	
	break;
	}
	// Update Color in Buffer
	finalColor.MaxToOne();

	const int px{ static_cast<int>(v.position.x) };
	const int py{ static_cast<int>(v.position.y) };

	m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
}


#pragma region W01
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
	for (auto& NDCvertex : vertices_ndc)
	{
		float ScreenSpaceVertexX = ((NDCvertex.position.x + 1) / 2) * m_Width;
		float ScreenSpaceVertexY = ((1 - NDCvertex.position.y) / 2) * m_Height;
		screenSpaceVertices.push_back({ { ScreenSpaceVertexX, ScreenSpaceVertexY, NDCvertex.position.z }, NDCvertex.color });
	}

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{ 0,0,0 };
			Vector3 pixel{ float(px) + 0.5f, float(py) + 0.5f , 0 }; // checking pixel from the center

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
			Vector2 pixel{ float(px) + 0.5f, float(py) + 0.5f };

			if (Utils::IsPointInTriangle(point, vertices))
			{
				const Vector2 v0{ vertices[0].position.x, vertices[0].position.y };
				const Vector2 v1{ vertices[1].position.x, vertices[1].position.y };
				const Vector2 v2{ vertices[2].position.x, vertices[2].position.y };

				float area = Vector2::Cross(v1 - v0, v2 - v0);

				float weight0 = Vector2::Cross(v2 - v1, pixel - v1) / area;
				float weight1 = Vector2::Cross(v0 - v2, pixel - v2) / area;
				float weight2 = Vector2::Cross(v1 - v0, pixel - v0) / area;

				//Interpolated Color 𝑽𝒆𝒓𝒕𝒆𝒙𝑪𝒐𝒍𝒐𝒓𝟎∗𝑾𝟎 + 𝑽𝒆𝒓𝒕𝒆𝒙𝑪𝒐𝒍𝒐𝒓𝟏∗𝑾𝟏 + 𝑽𝒆𝒓𝒕𝒆𝒙𝑪𝒐𝒍𝒐𝒓𝟐∗𝑾𝟐		
				finalColor = (vertices[0].color * weight0) + (vertices[1].color * weight1) + (vertices[2].color * weight2);
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

		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				ColorRGB finalColor{ 0,0,0 };

				Vector2 pixel{ float(px) + 0.5f, float(py) + 0.5f }; // checking pixel from the center
				Vector3 point{ float(px) + 0.5f, float(py) + 0.5f , 0 };
				const int pixelIdx{ px + py * m_Width };

				if (Utils::IsPointInTriangle(point, triangle))
				{
					float weight0 = Vector2::Cross(v2 - v1, pixel - v1) / area;
					float weight1 = Vector2::Cross(v0 - v2, pixel - v2) / area;
					float weight2 = Vector2::Cross(v1 - v0, pixel - v0) / area;


					const float depth0{ triangle[0].position.z };
					const float depth1{ triangle[1].position.z };
					const float depth2{ triangle[2].position.z };

					const float interpolatedDepth = depth0 * weight0 + depth1 * weight1 + depth2 * weight2;
					//
					//const float interpolatedDepth{ 1.f / (weight0 * (1.f / depth0) + weight1 * (1.f / depth1) + weight2 * (1.f / depth2)) };

					float depth = m_pDepthBufferPixels[pixelIdx];

					//if (depth < interpolatedDepth || interpolatedDepth < 0.f || interpolatedDepth > 1.f) continue;

					if (depth > interpolatedDepth)
					{
						m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;

						finalColor = (triangle[0].color * weight0) + (triangle[1].color * weight1) + (triangle[2].color * weight2);
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

		for (int px{ minX }; px < maxX; ++px)
		{
			for (int py{ minY }; py < maxY; ++py)
			{
				ColorRGB finalColor{ 0,0,0 };

				Vector2 pixel{ float(px) + 0.5f, float(py) + 0.5f }; // checking pixel from the center
				Vector3 point{ float(px) + 0.5f, float(py) + 0.5f , 0 };
				const int pixelIdx{ px + py * m_Width };

				if (Utils::IsPointInTriangle(point, triangle))
				{
					float weight0 = Vector2::Cross(v2 - v1, pixel - v1) / area;
					float weight1 = Vector2::Cross(v0 - v2, pixel - v2) / area;
					float weight2 = Vector2::Cross(v1 - v0, pixel - v0) / area;


					const float depth0{ triangle[0].position.z };
					const float depth1{ triangle[1].position.z };
					const float depth2{ triangle[2].position.z };

					const float interpolatedDepth = depth0 * weight0 + depth1 * weight1 + depth2 * weight2;
					//
					//const float interpolatedDepth{ 1.f / (weight0 * (1.f / depth0) + weight1 * (1.f / depth1) + weight2 * (1.f / depth2)) };

					float depth = m_pDepthBufferPixels[pixelIdx];

					//if (depth < interpolatedDepth || interpolatedDepth < 0.f || interpolatedDepth > 1.f) continue;

					if (depth > interpolatedDepth)
					{
						m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;

						finalColor = (triangle[0].color * weight0) + (triangle[1].color * weight1) + (triangle[2].color * weight2);
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
#pragma endregion
#pragma region W02
void Renderer::W2_QuadNoOptimization()
{
	std::vector<Vertex> vertices_world
	{
				Vertex{{ -3.0f,  3.0f, -2.0f}, {1,1,1}},
				Vertex{{  0.0f,  3.0f, -2.0f}, {1,1,1}},
				Vertex{{  3.0f,  3.0f, -2.0f}, {1,1,1}},
				Vertex{{ -3.0f,  0.0f, -2.0f}, {1,1,1}},
				Vertex{{  0.0f,  0.0f, -2.0f}, {1,1,1}},
				Vertex{{  3.0f,  0.0f, -2.0f}, {1,1,1}},
				Vertex{{ -3.0f, -3.0f, -2.0f}, {1,1,1}},
				Vertex{{  0.0f, -3.0f, -2.0f}, {1,1,1}},
				Vertex{{  3.0f, -3.0f, -2.0f}, {1,1,1}},
	};

	std::vector<Vertex> vertices{};
	VertexTransformationFunction(vertices_world, vertices);
	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);

	std::vector<Vertex> triangles
	{
		vertices[3], vertices[0], vertices[4],
		vertices[0], vertices[1], vertices[4],
		vertices[4], vertices[1], vertices[5],
		vertices[1], vertices[2], vertices[5],
		vertices[6], vertices[3], vertices[7],
		vertices[3], vertices[4], vertices[7],
		vertices[7], vertices[4], vertices[8],
		vertices[4], vertices[5], vertices[8],

	};

	for (int index{ 0 }; index < triangles.size(); index += 3)
	{

		std::vector<Vertex> triangle
		{
			triangles[index + 0],
			triangles[index + 1],
			triangles[index + 2],
		};

		Vertex v0 = triangle[0];
		Vertex v1 = triangle[1];
		Vertex v2 = triangle[2];

		const Vector2 V0{ v0.position.x, v0.position.y };
		const Vector2 V1{ v1.position.x, v1.position.y };
		const Vector2 V2{ v2.position.x, v2.position.y };

		float area = Vector2::Cross(V1 - V0, V2 - V0);

		//bounding box
		int minX = std::min({ v0.position.x, v1.position.x, v2.position.x });
		int maxX = std::max({ v0.position.x, v1.position.x, v2.position.x });;
		int minY = std::min({ v0.position.y, v1.position.y, v2.position.y });;
		int maxY = std::max({ v0.position.y, v1.position.y, v2.position.y });;

		if (minX < 0) minX = 0;
		if (maxX > m_Width) maxX = m_Width;

		if (minY < 0) minY = 0;
		if (maxY > m_Height) maxY = m_Height;

		for (int px{ minX }; px < maxX; ++px)
		{
			for (int py{ minY }; py < maxY; ++py)
			{
				ColorRGB finalColor{ 0,0,0 };

				Vector2 pixel{ float(px) + 0.5f, float(py) + 0.5f }; // checking pixel from the center
				Vector3 point{ float(px) + 0.5f, float(py) + 0.5f , 0 };
				const int pixelIdx{ px + py * m_Width };

				if (Utils::IsPointInTriangle(point, triangle))
				{
					float weight0 = Vector2::Cross(V2 - V1, pixel - V1) / area;
					float weight1 = Vector2::Cross(V0 - V2, pixel - V2) / area;
					float weight2 = Vector2::Cross(V1 - V0, pixel - V0) / area;


					const float depth0{ v0.position.z };
					const float depth1{ v1.position.z };
					const float depth2{ v2.position.z };

					const float interpolatedDepth = depth0 * weight0 + depth1 * weight1 + depth2 * weight2;
					//
					//const float interpolatedDepth{ 1.f / (weight0 * (1.f / depth0) + weight1 * (1.f / depth1) + weight2 * (1.f / depth2)) };

					float depth = m_pDepthBufferPixels[pixelIdx];

					//if (depth < interpolatedDepth || interpolatedDepth < 0.f || interpolatedDepth > 1.f) continue;

					if (depth > interpolatedDepth)
					{
						m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;

						finalColor = (v0.color * weight0) + (v1.color * weight1) + (v2.color * weight2);
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

void Renderer::W2_Quad()
{
	//Define Mesh
	//std::vector<Mesh> meshes_world
	//{
	//Mesh
	//{
	//	{
	//		Vertex{{ -3.0f,  3.0f, -2.0f},{},{ 0.0f, 0.0f}},
	//		Vertex{{  0.0f,  3.0f, -2.0f},{},{ 0.5f, 0.0f}},
	//		Vertex{{  3.0f,  3.0f, -2.0f},{},{ 1.0f, 0.0f}},
	//		Vertex{{ -3.0f,  0.0f, -2.0f},{},{ 0.0f, 0.5f}},
	//		Vertex{{  0.0f,  0.0f, -2.0f},{},{ 0.5f, 0.5f}},
	//		Vertex{{  3.0f,  0.0f, -2.0f},{},{ 1.0f, 0.5f}},
	//		Vertex{{ -3.0f, -3.0f, -2.0f},{},{ 0.0f, 1.0f}},
	//		Vertex{{  0.0f, -3.0f, -2.0f},{},{ 0.5f, 1.0f}},
	//		Vertex{{  3.0f, -3.0f, -2.0f},{},{ 1.0f, 1.0f}},
	//	},
	//	{
	//		3,0,1,   1,4,3,   4,1,2,
	//		2,5,4,   6,3,4,   4,7,6,
	//		7,4,5,   5,8,7,
	//	},
	//	PrimitiveTopology::TriangleList,
	//	}
	//};

	std::vector<Mesh> meshes_world
	{
	Mesh
	{
		{
			Vertex{{ -3.0f,  3.0f, -2.0f},{1,1,1},{ 0.0f, 0.0f}},
			Vertex{ {  0.0f,  3.0f, -2.0f},{1,1,1},{ 0.5f, 0.0f} },
			Vertex{ {  3.0f,  3.0f, -2.0f},{1,1,1},{ 1.0f, 0.0f} },
			Vertex{ { -3.0f,  0.0f, -2.0f},{1,1,1},{ 0.0f, 0.5f} },
			Vertex{ {  0.0f,  0.0f, -2.0f},{1,1,1},{ 0.5f, 0.5f} },
			Vertex{ {  3.0f,  0.0f, -2.0f},{1,1,1},{ 1.0f, 0.5f} },
			Vertex{ { -3.0f, -3.0f, -2.0f},{1,1,1},{ 0.0f, 1.0f} },
			Vertex{ {  0.0f, -3.0f, -2.0f},{1,1,1},{ 0.5f, 1.0f} },
			Vertex{ {  3.0f, -3.0f, -2.0f},{1,1,1},{ 1.0f, 1.0f} },
		},
		{
			3,0,4,1,5,2,
			2,6,
			6,3,7,4,8,5
		},
		PrimitiveTopology::TriangleStrip,
	},
	};



	for (auto& mesh : meshes_world)
	{
		VertexTransformationFunction(mesh);


		Vertex_Out v0, v1, v2;
		switch (mesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangleStrip:
		{
			for (int indicesIndex{}; indicesIndex < mesh.indices.size() - 2; ++indicesIndex)
			{

				if (indicesIndex % 2 == 0)
				{
					v0 = mesh.vertices_out[mesh.indices[indicesIndex]];
					v1 = mesh.vertices_out[mesh.indices[1 + indicesIndex]];
					v2 = mesh.vertices_out[mesh.indices[2 + indicesIndex]];
				}
				else
				{
					v0 = mesh.vertices_out[mesh.indices[indicesIndex]];
					v1 = mesh.vertices_out[mesh.indices[2 + indicesIndex]];
					v2 = mesh.vertices_out[mesh.indices[1 + indicesIndex]];
				}

				//clipping
				if (!(IsVertexInFrustrum(v0.position) || IsVertexInFrustrum(v1.position) || IsVertexInFrustrum(v2.position)))
				{
					NDCtoScreenSpace(v0, v1, v2);
					RenderTriangle(v0, v1, v2);
				}

			}
		}
		break;
		case PrimitiveTopology::TriangleList:
		{
			for (int indicesIndex{}; indicesIndex < mesh.indices.size(); ++indicesIndex)
			{
				v0 = mesh.vertices_out[mesh.indices[indicesIndex]];
				v1 = mesh.vertices_out[mesh.indices[++indicesIndex]];
				v2 = mesh.vertices_out[mesh.indices[++indicesIndex]];

				NDCtoScreenSpace(v0, v1, v2);
				RenderTriangle(v0, v1, v2);
			}
		}
		break;
		}

	}
}
//week 1 and 2, not correct depth
//void Renderer::RenderTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2) const
//{
//	std::vector<Vertex_Out> triangle
//	{
//		v0,
//		v1,
//		v2,
//	};
//
//	const Vector2 V0{ v0.position.x, v0.position.y };
//	const Vector2 V1{ v1.position.x, v1.position.y };
//	const Vector2 V2{ v2.position.x, v2.position.y };
//
//	float area = Vector2::Cross(V1 - V0, V2 - V0);
//
//	//bounding box
//	int minX = std::min({ v0.position.x, v1.position.x, v2.position.x });
//	int maxX = std::max({ v0.position.x, v1.position.x, v2.position.x });;
//	int minY = std::min({ v0.position.y, v1.position.y, v2.position.y });;
//	int maxY = std::max({ v0.position.y, v1.position.y, v2.position.y });;
//
//	// Add margin to prevent seethrough lines between quads
//	const float margin{ 1.f };
//	minX -= margin;
//	minY -= margin;
//	maxX += margin;
//	maxY += margin;
//
//	// Make sure the boundingbox is on the screen
//	minX = Clamp(minX, 0, m_Width);
//	minY = Clamp(minY, 0,m_Height);
//	maxX = Clamp(maxX, 0, m_Width);
//	maxY = Clamp(maxY, 0, m_Height);
//	
//	for (int px{ minX }; px < maxX; ++px)
//	{
//		for (int py{ minY }; py < maxY; ++py)
//		{
//			ColorRGB finalColor{ 0, 0, 0 };
//			ColorRGB fromTexture{};
//
//			Vector2 pixel{ float(px) + 0.5f, float(py) + 0.5f }; // checking pixel from the center
//			Vector3 point{ float(px) + 0.5f, float(py) + 0.5f , 0 };
//			const int pixelIdx{ px + py * m_Width };
//
//				//const bool isInTriangle = weight0 > 0 && weight1 > 0 && weight2 > 0;
//				if (Utils::IsPointInTriangle(point, triangle))
//				{
//					float weight0 = Vector2::Cross(V2 - V1, pixel - V1) / area;
//					float weight1 = Vector2::Cross(V0 - V2, pixel - V2) / area;
//					float weight2 = Vector2::Cross(V1 - V0, pixel - V0) / area;
//
//					//const float depth0{ v0.position.z };
//					//const float depth1{ v1.position.z };
//					//const float depth2{ v2.position.z };
//
//					//const float interpolatedW{ 1.f / ((weight0 * (1.f / v0.position.z) + (weight1 * (1.f / v1.position.z)) + (weight2 * (1.f / v2.position.z)))) };
//					//const float interpolatedDepth = CalculateInterpolatedZ(triangle, weight0, weight1, weight2); //interpolated Z
//
//					//const float interpolatedW{ 1.f / ((weight0 * (1.f / v0.position.w) + (weight1 * (1.f / v1.position.w)) + (weight2 * (1.f / v2.position.w)))) };
//					const float interpolatedDepth = CalculateInterpolatedW(triangle, weight0, weight1, weight2);
//
//					
//
//
//					float depth = m_pDepthBufferPixels[pixelIdx];
//
//					//if (interpolatedDepth < 0.0f || interpolatedDepth > 1.0f)
//						//continue;
//
//
//					switch (m_RenderMode)
//					{
//					case RenderMode::Texture:
//						if (depth > interpolatedDepth)
//						{
//							Vector2 interpolatedUV = ((triangle[0].uv / triangle[0].position.z) * weight0 + (triangle[1].uv / triangle[1].position.z) * weight1 + (triangle[2].uv / triangle[2].position.z) * weight2) * interpolatedDepth;
//
//							m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;
//
//
//							fromTexture = { m_pTexture->Sample(interpolatedUV) };
//
//							fromTexture.MaxToOne();
//
//							m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
//								static_cast<uint8_t>(fromTexture.r * 255),
//								static_cast<uint8_t>(fromTexture.g * 255),
//								static_cast<uint8_t>(fromTexture.b * 255));
//						}
//						break;
//					case RenderMode::Buffer:
//						if (depth > interpolatedDepth)
//						{
//							m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;
//
//							finalColor = (v0.color * weight0) + (v1.color * weight1) + (v2.color * weight2);
//							// Update Color in Buffer
//							finalColor.MaxToOne();
//
//							m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
//								static_cast<uint8_t>(finalColor.r * 255),
//								static_cast<uint8_t>(finalColor.g * 255),
//								static_cast<uint8_t>(finalColor.b * 255));
//						}
//						break;
//					}
//				}
//		}
//	}
//}

#pragma endregion
#pragma region W03
//void Renderer::RenderTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2) const
//{
//	// If a triangle has the same vertex twice, it means it has no surface and can't be rendered.
//	if (v0 == v1 || v1 == v2 || v2 == v0)
//	{
//		return;
//	}
//
//	std::vector<Vertex_Out> triangle
//	{
//		v0,
//		v1,
//		v2,
//	};
//
//	const Vector2 V0{ v0.position.x, v0.position.y };
//	const Vector2 V1{ v1.position.x, v1.position.y };
//	const Vector2 V2{ v2.position.x, v2.position.y };
//
//	//bounding box
//	int minX = std::min({ v0.position.x, v1.position.x, v2.position.x });
//	int maxX = std::max({ v0.position.x, v1.position.x, v2.position.x });;
//	int minY = std::min({ v0.position.y, v1.position.y, v2.position.y });;
//	int maxY = std::max({ v0.position.y, v1.position.y, v2.position.y });;
//
//	// Add margin to prevent seethrough lines between quads
//	const float margin{ 1.f };
//	minX -= margin;
//	minY -= margin;
//	maxX += margin;
//	maxY += margin;
//
//	// Make sure the boundingbox is on the screen
//	minX = Clamp(minX, 0, m_Width);
//	minY = Clamp(minY, 0, m_Height);
//	maxX = Clamp(maxX, 0, m_Width);
//	maxY = Clamp(maxY, 0, m_Height);
//
//	float area = Vector2::Cross(V1 - V0, V2 - V0);
//
//	for (int px{ minX }; px < maxX; ++px)
//	{
//		for (int py{ minY }; py < maxY; ++py)
//		{
//			Vector2 pixel{ float(px) + 0.5f, float(py) + 0.5f }; // checking pixel from the center
//			Vector3 point{ float(px) + 0.5f, float(py) + 0.5f , 0 };
//			const int pixelIdx{ px + py * m_Width };
//
//			//const bool isInTriangle = weight0 > 0 && weight1 > 0 && weight2 > 0;
//			if (Utils::IsPointInTriangle(point, triangle))
//			{
//				float weight0 = Vector2::Cross(V2 - V1, pixel - V1) / area;
//				float weight1 = Vector2::Cross(V0 - V2, pixel - V2) / area;
//				float weight2 = Vector2::Cross(V1 - V0, pixel - V0) / area;
//
//				const float depth0{ v0.position.z };
//				const float depth1{ v1.position.z };
//				const float depth2{ v2.position.z };
//
//				//const float interpolatedZ{ 1.f / ((weight0 * (1.f / v0.position.z) + (weight1 * (1.f / v1.position.z)) + (weight2 * (1.f / v2.position.z)))) };
//				const float interpolatedDepth = CalculateInterpolatedZ(triangle, weight0, weight1, weight2); //interpolated Z
//
//				
//				//makes sure that the object is not rendered if it is behind camera(otherwise mirrored)  (Frustum clipping)
//				float depth = m_pDepthBufferPixels[pixelIdx];
//				if (depth < interpolatedDepth || interpolatedDepth < 0.f || interpolatedDepth > 1.f) continue;
//				m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;
//
//				ColorRGB finalColor{ 0, 0, 0 };
//
//				switch (m_RenderMode)
//				{
//				case RenderMode::Texture:
//					{
//						Vector2 interpolatedUV = ((triangle[0].uv / triangle[0].position.z) * weight0 + (triangle[1].uv / triangle[1].position.z) * weight1 + (triangle[2].uv / triangle[2].position.z) * weight2) * interpolatedDepth;				
//						finalColor = { m_pTexture->Sample(interpolatedUV) };
//					}
//					break;
//				case RenderMode::Buffer:
//					{					
//						//finalColor = (v0.color * weight0) + (v1.color * weight1) + (v2.color * weight2); //just color no depth visualization
//						const float depthCol{ Remap(interpolatedDepth,0.990f,1.f) };
//						finalColor = { depthCol,depthCol,depthCol };
//					}
//					break;
//				}
//				// Update Color in Buffer
//				finalColor.MaxToOne();
//
//				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
//					static_cast<uint8_t>(finalColor.r * 255),
//					static_cast<uint8_t>(finalColor.g * 255),
//					static_cast<uint8_t>(finalColor.b * 255));
//			}
//		}
//	}
//	
//}
#pragma endregion

//void Renderer::Clipping( Vertex_Out& v0,  Vertex_Out& v1,  Vertex_Out& v2)
//{
//	std::vector <Vertex_Out> triangle{ v0, v1, v2 };
//	std::vector <Vertex_Out> insideVertices;
//	std::vector <Vertex_Out> outsideVertices;
//
//		for (const auto& vertex : triangle)
//		{
//			if (IsVertexInFrustrum(vertex.position) )
//			{
//				insideVertices.push_back(vertex);
//			}
//			else 
//			{
//				outsideVertices.push_back(vertex);
//			}
//		}
//
//		if (insideVertices.size() == 3) 
//		{
//			NDCtoScreenSpace(v0, v1, v2);
//			RenderTriangle(v0, v1, v2);
//		}
//		else if (insideVertices.size() == 2)
//		{
//			Vertex_Out intersectionPoints[2];
//
//			//calculate the intersection points
//			//
//			//
//			//
//
//			//NDC to screen space
//
//
//			//Render Triangles
//			RenderTriangle(insideVertices[0], intersectionPoints[1], intersectionPoints[0]);
//			RenderTriangle(intersectionPoints[1], insideVertices[0], insideVertices[1]);
//		}
//		else
//		{
//			
//		}		
//}

