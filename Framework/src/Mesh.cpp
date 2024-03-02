//--------------------------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------------------------
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <codecvt>
#include <cassert>

namespace {

	//----------------------------------------------------------------------------------------------------
	//	convert into UTF-8 string
	//----------------------------------------------------------------------------------------------------
	std::string ToUTF8(const std::wstring& value)
	{
		uint32_t length = WideCharToMultiByte(
			CP_UTF8, 0U, value.data(), -1, nullptr, 0, nullptr, nullptr);

		char* buffer = new char[length];

		WideCharToMultiByte(
			CP_UTF8, 0U, value.data(), -1, buffer, length, nullptr, nullptr);

		std::string result(buffer);
		delete[] buffer;
		buffer = nullptr;

		return result;
	}

	//----------------------------------------------------------------------------------------------------
	//	convert into std::wstring
	//----------------------------------------------------------------------------------------------------
	std::wstring Convert(const aiString& path)
	{
		wchar_t temp[256] = {};
		size_t size;
		mbstowcs_s(&size, temp, path.C_Str(), 256);
		return std::wstring(temp);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// MeshLoader class
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	class MeshLoader
	{
		//================================================================================================
		// list of friend classes and methods.
		//================================================================================================
		/* NOTHING */

	public:
		//================================================================================================
		// public variables.
		//================================================================================================
		/* NOTHING */

		//================================================================================================
		// public methods.
		//================================================================================================
		MeshLoader();
		virtual ~MeshLoader();

		bool Load(
			const wchar_t* filename,
			std::vector<Mesh>& meshes,
			std::vector<Material>& materials);

	private:
		//================================================================================================
		// private variables.
		//================================================================================================
		/* NOTHING */

		//================================================================================================
		// private methods.
		//================================================================================================
		void ParseMesh(Mesh& dstMesh, const aiMesh* pSrcMesh);
		void ParseMaterial(Material& dstMaterial, const aiMaterial* pSrcMaterial);
	};

	//----------------------------------------------------------------------------------------------------
	//	constructor
	//----------------------------------------------------------------------------------------------------
	MeshLoader::MeshLoader()
	{
		/* DO_NOTHING */
	}

	//----------------------------------------------------------------------------------------------------
	//	destructor
	//----------------------------------------------------------------------------------------------------
	MeshLoader::~MeshLoader()
	{
		/* DO_NOTHING */
	}

	//----------------------------------------------------------------------------------------------------
	//	load mesh
	//----------------------------------------------------------------------------------------------------
	bool MeshLoader::Load
	(
		const wchar_t* filename,
		std::vector<Mesh>& meshes,
		std::vector<Material>& materials
	)
	{
		if (filename == nullptr)
		{
			return false;
		}

		// convert from wchar_t to char(UTF-8)
		std::string path = ToUTF8(filename);

		Assimp::Importer importer;
		uint32_t flag = 0;
		flag |= aiProcess_Triangulate;
		flag |= aiProcess_PreTransformVertices;
		flag |= aiProcess_CalcTangentSpace;
		flag |= aiProcess_GenSmoothNormals;
		flag |= aiProcess_GenUVCoords;
		flag |= aiProcess_RemoveRedundantMaterials;
		flag |= aiProcess_OptimizeMeshes;

		// read file
		const aiScene* pScene = importer.ReadFile(path, flag);

		// check if it's null
		if (pScene == nullptr)
		{
			return false;
		}

		// allocate memory for mesh
		meshes.clear();
		meshes.resize(pScene->mNumMeshes);

		// convert mesh data
		for (size_t i = 0; i < meshes.size(); ++i)
		{
			const aiMesh* pMesh = pScene->mMeshes[i];
			ParseMesh(meshes[i], pMesh);
		}

		// allocate memory for material
		materials.clear();
		materials.resize(pScene->mNumMaterials);

		// convert mesh data
		for (size_t i = 0; i < materials.size(); ++i)
		{
			const aiMaterial* pMaterial = pScene->mMaterials[i];
			ParseMaterial(materials[i], pMaterial);
		}

		// clear as it's no longer necessary
		pScene = nullptr;

		// normally end
		return true;
	}

	//----------------------------------------------------------------------------------------------------
	//	analyze mesh data
	//----------------------------------------------------------------------------------------------------
	void MeshLoader::ParseMesh(Mesh& dstMesh, const aiMesh* pSrcMesh)
	{
		// set material index
		dstMesh.MaterialId = pSrcMesh->mMaterialIndex;

		aiVector3D zero3D(0.f, 0.f, 0.f);

		// allocate memory for vertex data
		dstMesh.Vertices.resize(pSrcMesh->mNumVertices);

		for (uint32_t i = 0u; i < pSrcMesh->mNumVertices; ++i)
		{
			aiVector3D* pPosition = &(pSrcMesh->mVertices[i]);
			aiVector3D* pNormal = &(pSrcMesh->mNormals[i]);
			aiVector3D* pTexCoord = (pSrcMesh->HasTextureCoords(0)) ? &(pSrcMesh->mTextureCoords[0][i]) : &zero3D;
			aiVector3D* pTangent = (pSrcMesh->HasTangentsAndBitangents()) ? &(pSrcMesh->mTangents[i]) : &zero3D;

			dstMesh.Vertices[i] = MeshVertex(
				DirectX::XMFLOAT3(pPosition->x, pPosition->y, pPosition->z),
				DirectX::XMFLOAT3(pNormal->x, pNormal->y, pNormal->z),
				DirectX::XMFLOAT2(pTexCoord->x, pTexCoord->y),
				DirectX::XMFLOAT3(pTangent->x, pTangent->y, pTangent->z)
			);
		}

		// allocate memory for indices of vertices
		dstMesh.Indices.resize(pSrcMesh->mNumFaces * 3);

		for (uint32_t i = 0u; i < pSrcMesh->mNumFaces; ++i)
		{
			const aiFace& face = pSrcMesh->mFaces[i];
			assert(face.mNumIndices == 3); // it's definitely 3 because we triangulated faces

			dstMesh.Indices[i * 3 + 0] = face.mIndices[0];
			dstMesh.Indices[i * 3 + 1] = face.mIndices[1];
			dstMesh.Indices[i * 3 + 2] = face.mIndices[2];
		}
	}

	//----------------------------------------------------------------------------------------------------
	//	analyze material data
	//----------------------------------------------------------------------------------------------------
	void MeshLoader::ParseMaterial(Material& dstMaterial, const aiMaterial* pSrcMaterial)
	{
		// diffuse reflection
		{
			aiColor3D color(0.f, 0.f, 0.f);

			if (pSrcMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
			{
				dstMaterial.Diffuse.x = color.r;
				dstMaterial.Diffuse.y = color.g;
				dstMaterial.Diffuse.z = color.b;
			}
			else
			{
				dstMaterial.Diffuse.x = 0.5f;
				dstMaterial.Diffuse.y = 0.5f;
				dstMaterial.Diffuse.z = 0.5f;
			}
		}

		// specular reflection
		{
			aiColor3D color(0.f, 0.f, 0.f);

			if (pSrcMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
			{
				dstMaterial.Specular.x = color.r;
				dstMaterial.Specular.y = color.g;
				dstMaterial.Specular.z = color.b;
			}
			else
			{
				dstMaterial.Specular.x = 0.f;
				dstMaterial.Specular.y = 0.f;
				dstMaterial.Specular.z = 0.f;
			}
		}

		// shininess(intensity of specular reflection)
		{
			float shininess = 0.f;
			if (pSrcMaterial->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
			{
				dstMaterial.Shininess = shininess;
			}
			else
			{
				dstMaterial.Shininess = 0.f;
			}
		}

		// diffuse map
		{
			aiString path;
			if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) == AI_SUCCESS)
			{
				dstMaterial.DiffuseMap = std::string(path.C_Str());
			}
			else
			{
				dstMaterial.DiffuseMap.clear();
			}
		}
	}

} // namespace

//--------------------------------------------------------------------------------------------------------
// Constant Values.
//--------------------------------------------------------------------------------------------------------
#define FMT_FLOAT3 DXGI_FORMAT_R32G32B32_FLOAT
#define FMT_FLOAT2 DXGI_FORMAT_R32G32_FLOAT
#define APPEND D3D12_APPEND_ALIGNED_ELEMENT
#define IL_VERTEX D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA

const D3D12_INPUT_ELEMENT_DESC MeshVertex::InputElements[] = {
	{ "POSITION", 0, FMT_FLOAT3, 0, APPEND, IL_VERTEX, 0 },
	{ "NORMAL" ,  0, FMT_FLOAT3, 0, APPEND, IL_VERTEX, 0 },
	{ "TEXCOORD", 0, FMT_FLOAT2, 0, APPEND, IL_VERTEX, 0 },
	{ "TANGENT",  0, FMT_FLOAT3, 0, APPEND, IL_VERTEX, 0 }
};
const D3D12_INPUT_LAYOUT_DESC MeshVertex::InputLayout = {
	MeshVertex::InputElements,
	MeshVertex::inputElementCount
};
static_assert(sizeof(MeshVertex) == 44, "Vertex struct/layout mismatch");

#undef FMT_FLOAT3
#undef FMT_FLOAT2
#undef APPEND
#undef IL_VERTEX

//--------------------------------------------------------------------------------------------------------
//	load mesh
//--------------------------------------------------------------------------------------------------------
bool LoadMesh
(
	const wchar_t* filename,
	std::vector<Mesh>& meshes,
	std::vector<Material>& materials
)
{
	MeshLoader loader;
	return loader.Load(filename, meshes, materials);
}
