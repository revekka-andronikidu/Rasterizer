#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"
#include <iostream>

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
		float aspectRatio{ 1.f };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		float nearPlane{ 0.1f };
		float farPlane{ 100.f };

		const float movementSpeed{ 5.f };

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f}, float _aspectRatio = 1.f)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;

			aspectRatio = _aspectRatio;

			CalculateProjectionMatrix();
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
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			//...

			KeyboardInput(deltaTime);
			MouseInput(deltaTime);

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}

		

		void KeyboardInput(float deltaTime)
		{
			const float speed{ 15.f };
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			if (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP])
			{
				origin += up * -deltaTime * speed;
			}
			if (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN])
			{
				origin -= up * -deltaTime * speed;
			}
			if (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT])
			{
				origin += right * -deltaTime * speed;
			}
			if (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT])
			{
				origin -= right * -deltaTime * speed;
			}

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
				
				totalYaw += deltaTime * static_cast<float>(mouseX) ;
				totalPitch += deltaTime * static_cast<float>(mouseY);

				const Matrix finalRotation{ Matrix::CreateRotation(totalPitch, totalYaw, 0) };
				forward = finalRotation.TransformVector(Vector3::UnitZ);
			}
		}
	};
}
