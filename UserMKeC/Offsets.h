#pragma once
#include <cstddef>
#include<vector>
#include <cmath>
#include <numbers>



struct vec4
{
    float w, x, y, z;
};

struct vec3
{
    float x, y, z;
    vec3 operator+(vec3 other)
    {
        return { this->x + other.x, this->y + other.y, this->z + other.z };
    }
    vec3 operator-(vec3 other)
    {
        return { this->x - other.x, this->y - other.y, this->z - other.z };
    }

    vec3 RelativeAngle()
    {
        return {
            std::atan2(-z, std::hypot(x, y)) * (180.0f / std::numbers::pi_v<float>),
            std::atan2(y, x) * (180.0f / std::numbers::pi_v<float>),
            0.0f
        };
    }

};

struct vec2
{
    float x, y;
};


namespace client_dll{
constexpr std::ptrdiff_t dwCSGOInput = 0x1A3B7E0;
constexpr std::ptrdiff_t dwEntityList = 0x19CFC48;
constexpr std::ptrdiff_t dwGameEntitySystem = 0x1AE9B58;
constexpr std::ptrdiff_t dwGameEntitySystem_highestEntityIndex = 0x1520;
constexpr std::ptrdiff_t dwGameRules = 0x1A2D668;
constexpr std::ptrdiff_t dwGlobalVars = 0x1828CD0;
constexpr std::ptrdiff_t dwGlowManager = 0x1A2CDA0;
constexpr std::ptrdiff_t dwLocalPlayerController = 0x1A1F8F0;
constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x1834B18;
constexpr std::ptrdiff_t dwPlantedC4 = 0x1A372A0;
constexpr std::ptrdiff_t dwPrediction = 0x18349B0;
constexpr std::ptrdiff_t dwSensitivity = 0x1A2E388;
constexpr std::ptrdiff_t dwSensitivity_sensitivity = 0x40;
constexpr std::ptrdiff_t dwViewAngles = 0x1A3BBB0;
constexpr std::ptrdiff_t dwViewMatrix = 0x1A31D30;
constexpr std::ptrdiff_t dwViewRender = 0x1A32540;
constexpr std::ptrdiff_t dwWeaponC4 = 0x19D2D60;
} // namespace client.dll


namespace C_BaseModelEntity {
    constexpr std::ptrdiff_t m_CRenderComponent = 0xA50; // CRenderComponent*
    constexpr std::ptrdiff_t m_CHitboxComponent = 0xA58; // CHitboxComponent
    constexpr std::ptrdiff_t m_LastHitGroup = 0xA80; // HitGroup_t
    constexpr std::ptrdiff_t m_bInitModelEffects = 0xAA8; // bool
    constexpr std::ptrdiff_t m_bIsStaticProp = 0xAA9; // bool
    constexpr std::ptrdiff_t m_nLastAddDecal = 0xAAC; // int32
    constexpr std::ptrdiff_t m_nDecalsAdded = 0xAB0; // int32
    constexpr std::ptrdiff_t m_iOldHealth = 0xAB4; // int32
    constexpr std::ptrdiff_t m_nRenderMode = 0xAB8; // RenderMode_t
    constexpr std::ptrdiff_t m_nRenderFX = 0xAB9; // RenderFx_t
    constexpr std::ptrdiff_t m_bAllowFadeInView = 0xABA; // bool
    constexpr std::ptrdiff_t m_clrRender = 0xAD8; // Color
    constexpr std::ptrdiff_t m_vecRenderAttributes = 0xAE0; // C_UtlVectorEmbeddedNetworkVar<EntityRenderAttribute_t>
    constexpr std::ptrdiff_t m_bRenderToCubemaps = 0xB48; // bool
    constexpr std::ptrdiff_t m_bNoInterpolate = 0xB49; // bool
    constexpr std::ptrdiff_t m_Collision = 0xB50; // CCollisionProperty
    constexpr std::ptrdiff_t m_Glow = 0xC00; // CGlowProperty
    constexpr std::ptrdiff_t m_flGlowBackfaceMult = 0xC58; // float32
    constexpr std::ptrdiff_t m_fadeMinDist = 0xC5C; // float32
    constexpr std::ptrdiff_t m_fadeMaxDist = 0xC60; // float32
    constexpr std::ptrdiff_t m_flFadeScale = 0xC64; // float32
    constexpr std::ptrdiff_t m_flShadowStrength = 0xC68; // float32
    constexpr std::ptrdiff_t m_nObjectCulling = 0xC6C; // uint8
    constexpr std::ptrdiff_t m_nAddDecal = 0xC70; // int32
    constexpr std::ptrdiff_t m_vDecalPosition = 0xC74; // Vector
    constexpr std::ptrdiff_t m_vDecalForwardAxis = 0xC80; // Vector
    constexpr std::ptrdiff_t m_flDecalHealBloodRate = 0xC8C; // float32
    constexpr std::ptrdiff_t m_flDecalHealHeightRate = 0xC90; // float32
    constexpr std::ptrdiff_t m_ConfigEntitiesToPropagateMaterialDecalsTo = 0xC98; // C_NetworkUtlVectorBase<CHandle<C_BaseModelEntity>>
    constexpr std::ptrdiff_t m_vecViewOffset = 0xCB0; // CNetworkViewOffsetVector
    constexpr std::ptrdiff_t m_pClientAlphaProperty = 0xCE0; // CClientAlphaProperty*
    constexpr std::ptrdiff_t m_ClientOverrideTint = 0xCE8; // Color
    constexpr std::ptrdiff_t m_bUseClientOverrideTint = 0xCEC; // bool
}

namespace C_BasePlayerPawn {
    constexpr std::ptrdiff_t m_pWeaponServices = 0x11A8; // CPlayer_WeaponServices*
    constexpr std::ptrdiff_t m_pItemServices = 0x11B0; // CPlayer_ItemServices*
    constexpr std::ptrdiff_t m_pAutoaimServices = 0x11B8; // CPlayer_AutoaimServices*
    constexpr std::ptrdiff_t m_pObserverServices = 0x11C0; // CPlayer_ObserverServices*
    constexpr std::ptrdiff_t m_pWaterServices = 0x11C8; // CPlayer_WaterServices*
    constexpr std::ptrdiff_t m_pUseServices = 0x11D0; // CPlayer_UseServices*
    constexpr std::ptrdiff_t m_pFlashlightServices = 0x11D8; // CPlayer_FlashlightServices*
    constexpr std::ptrdiff_t m_pCameraServices = 0x11E0; // CPlayer_CameraServices*
    constexpr std::ptrdiff_t m_pMovementServices = 0x11E8; // CPlayer_MovementServices*
    constexpr std::ptrdiff_t m_ServerViewAngleChanges = 0x11F8; // C_UtlVectorEmbeddedNetworkVar<ViewAngleServerChange_t>
    constexpr std::ptrdiff_t m_nHighestConsumedServerViewAngleChangeIndex = 0x1248; // uint32
    constexpr std::ptrdiff_t v_angle = 0x124C; // QAngle
    constexpr std::ptrdiff_t v_anglePrevious = 0x1258; // QAngle
    constexpr std::ptrdiff_t m_iHideHUD = 0x1264; // uint32
    constexpr std::ptrdiff_t m_skybox3d = 0x1268; // sky3dparams_t
    constexpr std::ptrdiff_t m_flDeathTime = 0x12F8; // GameTime_t
    constexpr std::ptrdiff_t m_vecPredictionError = 0x12FC; // Vector
    constexpr std::ptrdiff_t m_flPredictionErrorTime = 0x1308; // GameTime_t
    constexpr std::ptrdiff_t m_vecLastCameraSetupLocalOrigin = 0x130C; // Vector
    constexpr std::ptrdiff_t m_flLastCameraSetupTime = 0x1318; // GameTime_t
    constexpr std::ptrdiff_t m_flFOVSensitivityAdjust = 0x131C; // float32
    constexpr std::ptrdiff_t m_flMouseSensitivity = 0x1320; // float32
    constexpr std::ptrdiff_t m_vOldOrigin = 0x1324; // Vector
    constexpr std::ptrdiff_t m_flOldSimulationTime = 0x1330; // float32
    constexpr std::ptrdiff_t m_nLastExecutedCommandNumber = 0x1334; // int32
    constexpr std::ptrdiff_t m_nLastExecutedCommandTick = 0x1338; // int32
    constexpr std::ptrdiff_t m_hController = 0x133C; // CHandle<CBasePlayerController>
    constexpr std::ptrdiff_t m_bIsSwappingToPredictableController = 0x1340; // bool
}

namespace C_BaseEntity {
    constexpr std::ptrdiff_t m_CBodyComponent = 0x38; // CBodyComponent*
    constexpr std::ptrdiff_t m_NetworkTransmitComponent = 0x40; // CNetworkTransmitComponent
    constexpr std::ptrdiff_t m_nLastThinkTick = 0x320; // GameTick_t
    constexpr std::ptrdiff_t m_pGameSceneNode = 0x328; // CGameSceneNode*
    constexpr std::ptrdiff_t m_pRenderComponent = 0x330; // CRenderComponent*
    constexpr std::ptrdiff_t m_pCollision = 0x338; // CCollisionProperty*
    constexpr std::ptrdiff_t m_iMaxHealth = 0x340; // int32
    constexpr std::ptrdiff_t m_iHealth = 0x344; // int32
    constexpr std::ptrdiff_t m_lifeState = 0x348; // uint8
    constexpr std::ptrdiff_t m_bTakesDamage = 0x349; // bool
    constexpr std::ptrdiff_t m_nTakeDamageFlags = 0x350; // TakeDamageFlags_t
    constexpr std::ptrdiff_t m_nPlatformType = 0x358; // EntityPlatformTypes_t
    constexpr std::ptrdiff_t m_ubInterpolationFrame = 0x359; // uint8
    constexpr std::ptrdiff_t m_hSceneObjectController = 0x35C; // CHandle<C_BaseEntity>
    constexpr std::ptrdiff_t m_nNoInterpolationTick = 0x360; // int32
    constexpr std::ptrdiff_t m_nVisibilityNoInterpolationTick = 0x364; // int32
    constexpr std::ptrdiff_t m_flProxyRandomValue = 0x368; // float32
    constexpr std::ptrdiff_t m_iEFlags = 0x36C; // int32
    constexpr std::ptrdiff_t m_nWaterType = 0x370; // uint8
    constexpr std::ptrdiff_t m_bInterpolateEvenWithNoModel = 0x371; // bool
    constexpr std::ptrdiff_t m_bPredictionEligible = 0x372; // bool
    constexpr std::ptrdiff_t m_bApplyLayerMatchIDToModel = 0x373; // bool
    constexpr std::ptrdiff_t m_tokLayerMatchID = 0x374; // CUtlStringToken
    constexpr std::ptrdiff_t m_nSubclassID = 0x378; // CUtlStringToken
    constexpr std::ptrdiff_t m_nSimulationTick = 0x388; // int32
    constexpr std::ptrdiff_t m_iCurrentThinkContext = 0x38C; // int32
    constexpr std::ptrdiff_t m_aThinkFunctions = 0x390; // CUtlVector<thinkfunc_t>
    constexpr std::ptrdiff_t m_bDisabledContextThinks = 0x3A8; // bool
    constexpr std::ptrdiff_t m_flAnimTime = 0x3AC; // float32
    constexpr std::ptrdiff_t m_flSimulationTime = 0x3B0; // float32
    constexpr std::ptrdiff_t m_nSceneObjectOverrideFlags = 0x3B4; // uint8
    constexpr std::ptrdiff_t m_bHasSuccessfullyInterpolated = 0x3B5; // bool
    constexpr std::ptrdiff_t m_bHasAddedVarsToInterpolation = 0x3B6; // bool
    constexpr std::ptrdiff_t m_bRenderEvenWhenNotSuccessfullyInterpolated = 0x3B7; // bool
    constexpr std::ptrdiff_t m_nInterpolationLatchDirtyFlags = 0x3B8; // int32[2]
    constexpr std::ptrdiff_t m_ListEntry = 0x3C0; // uint16[11]
    constexpr std::ptrdiff_t m_flCreateTime = 0x3D8; // GameTime_t
    constexpr std::ptrdiff_t m_flSpeed = 0x3DC; // float32
    constexpr std::ptrdiff_t m_EntClientFlags = 0x3E0; // uint16
    constexpr std::ptrdiff_t m_bClientSideRagdoll = 0x3E2; // bool
    constexpr std::ptrdiff_t m_iTeamNum = 0x3E3; // uint8
    constexpr std::ptrdiff_t m_spawnflags = 0x3E4; // uint32
    constexpr std::ptrdiff_t m_nNextThinkTick = 0x3E8; // GameTick_t
    constexpr std::ptrdiff_t m_fFlags = 0x3EC; // uint32
    constexpr std::ptrdiff_t m_vecAbsVelocity = 0x3F0; // Vector
    constexpr std::ptrdiff_t m_vecVelocity = 0x400; // CNetworkVelocityVector
    constexpr std::ptrdiff_t m_vecBaseVelocity = 0x430; // Vector
    constexpr std::ptrdiff_t m_hEffectEntity = 0x43C; // CHandle<C_BaseEntity>
    constexpr std::ptrdiff_t m_hOwnerEntity = 0x440; // CHandle<C_BaseEntity>
    constexpr std::ptrdiff_t m_MoveCollide = 0x444; // MoveCollide_t
    constexpr std::ptrdiff_t m_MoveType = 0x445; // MoveType_t
    constexpr std::ptrdiff_t m_nActualMoveType = 0x446; // MoveType_t
    constexpr std::ptrdiff_t m_flWaterLevel = 0x448; // float32
    constexpr std::ptrdiff_t m_fEffects = 0x44C; // uint32
    constexpr std::ptrdiff_t m_hGroundEntity = 0x450; // CHandle<C_BaseEntity>
    constexpr std::ptrdiff_t m_nGroundBodyIndex = 0x454; // int32
    constexpr std::ptrdiff_t m_flFriction = 0x458; // float32
    constexpr std::ptrdiff_t m_flElasticity = 0x45C; // float32
    constexpr std::ptrdiff_t m_flGravityScale = 0x460; // float32
    constexpr std::ptrdiff_t m_flTimeScale = 0x464; // float32
    constexpr std::ptrdiff_t m_bAnimatedEveryTick = 0x468; // bool
    constexpr std::ptrdiff_t m_flNavIgnoreUntilTime = 0x46C; // GameTime_t
    constexpr std::ptrdiff_t m_hThink = 0x470; // uint16
    constexpr std::ptrdiff_t m_fBBoxVisFlags = 0x480; // uint8
    constexpr std::ptrdiff_t m_bPredictable = 0x481; // bool
    constexpr std::ptrdiff_t m_bRenderWithViewModels = 0x482; // bool
    constexpr std::ptrdiff_t m_nSplitUserPlayerPredictionSlot = 0x484; // CSplitScreenSlot
    constexpr std::ptrdiff_t m_nFirstPredictableCommand = 0x488; // int32
    constexpr std::ptrdiff_t m_nLastPredictableCommand = 0x48C; // int32
    constexpr std::ptrdiff_t m_hOldMoveParent = 0x490; // CHandle<C_BaseEntity>
    constexpr std::ptrdiff_t m_Particles = 0x498; // CParticleProperty
    constexpr std::ptrdiff_t m_vecPredictedScriptFloats = 0x4C0; // CUtlVector<float32>
    constexpr std::ptrdiff_t m_vecPredictedScriptFloatIDs = 0x4D8; // CUtlVector<int32>
    constexpr std::ptrdiff_t m_nNextScriptVarRecordID = 0x508; // int32
    constexpr std::ptrdiff_t m_vecAngVelocity = 0x518; // QAngle
    constexpr std::ptrdiff_t m_DataChangeEventRef = 0x524; // int32
    constexpr std::ptrdiff_t m_dependencies = 0x528; // CUtlVector<CEntityHandle>
    constexpr std::ptrdiff_t m_nCreationTick = 0x540; // int32
    constexpr std::ptrdiff_t m_bAnimTimeChanged = 0x54D; // bool
    constexpr std::ptrdiff_t m_bSimulationTimeChanged = 0x54E; // bool
    constexpr std::ptrdiff_t m_sUniqueHammerID = 0x558; // CUtlString
    constexpr std::ptrdiff_t m_nBloodType = 0x560; // BloodType
}


struct viewMatrix_t {
	float matrix[16];
};

namespace esp {
    inline std::vector<uintptr_t> entities = {};
    inline viewMatrix_t vm = {};

    inline uintptr_t pID;
    inline uintptr_t modBase;

    void frame();
    void render();
    void loop();

    bool w2s(const vec3& world, vec2& screen, float matrix[16]);
}