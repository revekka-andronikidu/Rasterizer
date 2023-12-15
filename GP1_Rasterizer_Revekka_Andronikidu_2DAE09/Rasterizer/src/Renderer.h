#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	struct Vertex_Out;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const;
		void VertexTransformationFunction(Mesh& meshes);

	

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		float m_AspectRatio{};

		bool m_F4Held{false};
		bool m_F5Held{ false };
		bool m_F6Held{ false };
		bool m_F7Held{ false };

		bool m_EnableNormalMap{ true };
		bool m_EnableRotating{ false };

		
		Texture* m_pDiffuseTexture;
		Texture* m_pSpecularTexture;
		Texture* m_pGlossinessTexture;
		Texture* m_pNormalTexture;

		const float m_SpecularShininess{ 25.0f };

		enum class RenderMode
		{
			Texture,
			Buffer,
			END
		};

		enum class ShadingMode
		{
			ObservedArea,	
			Diffuse,		
			Specular,		
			Combined,
			Ambient,
			END
		};

		

		RenderMode m_RenderMode{RenderMode::Texture};
		ShadingMode m_ShadingMode{ShadingMode::Combined};

		Mesh* m_pMesh;


		//private functions
		void RasterizationOnly();
		void ProjectionStage();
		void BarycenticCoordinates();
		void DepthBuffer();
		void BoundingBoxOptimization();
		void W2_QuadNoOptimization();
		void W2_Quad();

		void RenderTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2) const;
		void NDCtoScreenSpace(Vertex_Out& v0, Vertex_Out& v1, Vertex_Out& v2);
		void RenderMeshes(std::vector<Mesh> meshes_world);
		void PixelShading(const Vertex_Out& v) const;
		//void Clipping( Vertex_Out& v0,  Vertex_Out& v1,  Vertex_Out& v2);
		
		
	};
}


