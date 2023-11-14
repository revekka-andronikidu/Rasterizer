#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		const float movementSpeed{ 3.f };

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			//ONB => invViewMatrix
			//Inverse(ONB) => ViewMatrix

			right = Vector3::Cross(Vector3::UnitY, forward);
			up = Vector3::Cross(forward, right);

			invViewMatrix =
			{
				right,
				up,
				forward,
				origin
			};

			viewMatrix = invViewMatrix.Inverse();
			
			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
			// 
			//viewMatrix = Matrix::CreateLookAtLH();
		}

		void CalculateProjectionMatrix()
		{
			//TODO W3

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			//...

			KeyboardInput();
			MouseInput(deltaTime);

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}

		void KeyboardInput()
		{

		}
		void MouseInput(float deltaTime)
		{
			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
				{
					origin += up * -deltaTime * float(mouseY) * movementSpeed;
				}
				else
				{
					origin += forward * (-mouseY * movementSpeed * deltaTime);
					origin += right * (-mouseX * movementSpeed * deltaTime);
				}
			}

			if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT) && !(mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
			{

				origin += up * (-mouseY * movementSpeed * deltaTime);
				origin += right * (-mouseX * movementSpeed * deltaTime);
			}
		}
	};
}
