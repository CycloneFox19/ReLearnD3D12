#pragma once

//--------------------------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------------------------
#define NOMINMAX
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <cstdint>

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// MeshVertex structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MeshVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexCoord;
	DirectX::XMFLOAT3 Tangent;

	MeshVertex() = default;

	MeshVertex(
		DirectX::XMFLOAT3 const& position,
		DirectX::XMFLOAT3 const& normal,
		DirectX::XMFLOAT2 const& texcoord,
		DirectX::XMFLOAT3 const& tangent)
		: Position(position)
		, Normal(normal)
		, TexCoord(texcoord)
		, Tangent(tangent)
	{
		/* DO_NOTHING */
	}

	static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
	static const uint32_t inputElementCount = 4;
	static const D3D12_INPUT_ELEMENT_DESC InputElements[inputElementCount];
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Material structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Material
{
	DirectX::XMFLOAT3 Diffuse; //!< diffuse reflection
	DirectX::XMFLOAT3 Specular; //!< specular reflection
	float Alpha; //!< opacity
	float Shininess; //!< intensity of specular reflection
	std::string DiffuseMap; //!< path for texture file
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mesh structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Mesh
{
	std::vector<MeshVertex> Vertices; //!< vertex data
	std::vector<uint32_t> Indices; //!< indices of vertices
	uint32_t MaterialId; //!< material index
};

//--------------------------------------------------------------------------------------------------------
//! @brief load mesh
//! 
//! @param[in] filename name of file to load
//! @param[out] meshes container of mesh
//! @param[out] materials container of material
//! @retval true successfully loaded
//! @retval false failed to load
//--------------------------------------------------------------------------------------------------------
bool LoadMesh(
	const wchar_t* filename,
	std::vector<Mesh>& meshes,
	std::vector<Material>& materials);
