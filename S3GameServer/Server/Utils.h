#ifndef UTILS
#define UTILS
#pragma once
#include "../SDK/SDK.hpp"

using namespace SDK;
using namespace std;
enum class ESpawnActorNameMode : uint8_t
{
    Required_Fatal,
    Required_ErrorAndReturnNull,
    Required_ReturnNull,
    Requested
};

struct FActorSpawnParameters
{
    FName Name = FName(0);
    UObject* Template = nullptr;
    UObject* Owner = nullptr;
    UObject* Instigator = nullptr;
    UObject* OverrideLevel = nullptr;
    UObject* OverrideParentComponent;
    ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;
    ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;
    uint8	bRemoteOwned : 1;
    uint8	bNoFail : 1;
    uint8	bDeferConstruction : 1;
    uint8	bAllowDuringConstructionScript : 1;
    uint8	bForceGloballyUniqueName : 1;
    ESpawnActorNameMode NameMode;
    EObjectFlags ObjectFlags;
    void* CallbackSum_Callable;
    void* CallbackSum_HeapAllocation;
};

void VHook(void* Base, uintptr_t Index, void* Detour, void** Original = nullptr) {
    auto VTable = (*(void***)Base);
    if (!VTable)
        return;

    if (!VTable[Index])
        return;

    if (Original)
        *Original = VTable[Index];

    DWORD dwOld;
    VirtualProtect(&VTable[Index], 8, PAGE_EXECUTE_READWRITE, &dwOld);
    VTable[Index] = Detour;
    DWORD dwTemp;
    VirtualProtect(&VTable[Index], 8, dwOld, &dwTemp);
}

template<class T>
inline T* Cast(UObject* Src)
{
    return Src && (Src->IsA(T::StaticClass())) ? (T*)Src : nullptr;
}

template <typename _It>
static _It* GetInterface(UObject* Object)
{
    return ((_It * (*)(UObject*, UClass*)) (InSDKUtils::GetImageBase() + 0x15AB2B8))(Object, _It::StaticClass());
}

inline void* (__fastcall* GetInterfaceAddress)(UObject*, UClass*) = decltype(GetInterfaceAddress)(InSDKUtils::GetImageBase() + 0x15AB2B8);
inline UObject* (*StaticFindObject_)(UClass* Class, UObject* Package, const wchar_t* OrigInName, bool ExactClass) = decltype(StaticFindObject_)(InSDKUtils::GetImageBase() + 0x146D58C);
inline UObject* (*StaticLoadObject_)(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, uint32_t LoadFlags, UObject* Sandbox, bool bAllowObjectReconciliation, void* InstancingContext) = decltype(StaticLoadObject_)(InSDKUtils::GetImageBase() + 0x1E9B2D0);
inline AActor* (*InternalSpawnActor)(UWorld* World, UClass* Class, FTransform const* Transform, FActorSpawnParameters* SpawnParams) = decltype(InternalSpawnActor)(InSDKUtils::GetImageBase() + 0x1A8087C);
template <typename T>
inline T* StaticFindObject(std::string ObjectPath, UClass* Class = UObject::FindClassFast("Object"), UObject* Package = nullptr)
{
	return (T*)StaticFindObject_(Class, Package, std::wstring(ObjectPath.begin(), ObjectPath.end()).c_str(), false);
}

template <typename T>
inline T* StaticLoadObject(std::string Path, UClass* InClass = T::StaticClass(), UObject* Outer = nullptr)
{
	return (T*)StaticLoadObject_(InClass, Outer, std::wstring(Path.begin(), Path.end()).c_str(), nullptr, 0, nullptr, false, nullptr);
}

#define INV_PI			(0.31830988618f)
#define HALF_PI			(1.57079632679f)
#define PI 					(3.1415926535897932f)

float RadiansToDegrees(float Radians)
{
    return Radians * (180.0f / PI);
}
inline void SinCos(float* ScalarSin, float* ScalarCos, float  Value)
{
    float quotient = (INV_PI * 0.5f) * Value;
    if (Value >= 0.0f)
    {
        quotient = (float)((int)(quotient + 0.5f));
    }
    else
    {
        quotient = (float)((int)(quotient - 0.5f));
    }
    float y = Value - (2.0f * PI) * quotient;

    float sign;
    if (y > HALF_PI)
    {
        y = PI - y;
        sign = -1.0f;
    }
    else if (y < -HALF_PI)
    {
        y = -PI - y;
        sign = -1.0f;
    }
    else
    {
        sign = +1.0f;
    }

    float y2 = y * y;

    *ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

    float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
    *ScalarCos = sign * p;
}

inline FVector RotatorToVector(FRotator Rotator)
{
    float CP, SP, CY, SY;
    SinCos(&SP, &CP, Rotator.pitch * (PI / 180.0f));
    SinCos(&SY, &CY, Rotator.Yaw * (PI / 180.0f));

    FVector NewV = FVector(CP * CY, CP * SY, SP);

    return NewV;
}

inline FQuat FRotToQuat(FRotator Rot)
{
    const float DEG_TO_RAD = PI / 180.f;
    const float DIVIDE_BY_2 = DEG_TO_RAD / 2.f;
    float SP, SY, SR;
    float CP, CY, CR;

    SinCos(&SP, &CP, Rot.pitch * DIVIDE_BY_2);
    SinCos(&SY, &CY, Rot.Yaw * DIVIDE_BY_2);
    SinCos(&SR, &CR, Rot.Roll * DIVIDE_BY_2);

    FQuat RotationQuat;
    RotationQuat.X = CR * SP * SY - SR * CP * CY;
    RotationQuat.Y = -CR * SP * CY - SR * CP * SY;
    RotationQuat.Z = CR * CP * SY - SR * SP * CY;
    RotationQuat.W = CR * CP * CY + SR * SP * SY;

    return RotationQuat;
}

AActor* SpawnActor(UClass* Class, FTransform& Transform, AActor* Owner = nullptr)
{
    FActorSpawnParameters Params{};

    Params.Owner = Owner;
    Params.bDeferConstruction = false;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    return InternalSpawnActor(UWorld::GetWorld(), Class, &Transform, &Params);
}
inline AActor* SpawnActor(FVector Location, FRotator Rotation, UClass* Class)
{
    FTransform Transform{};
    Transform.Rotation = FRotToQuat(Rotation);
    Transform.Translation = Location;
    Transform.Scale3D = FVector{ 1,1,1 };

    return SpawnActor(Class, Transform);
}
template<typename T>
inline T* SpawnActor(FVector Loc, FRotator Rot = FRotator(), AActor* Owner = nullptr, SDK::UClass* Class = T::StaticClass())
{
    FTransform Transform{};
    Transform.Scale3D = FVector{ 1,1,1 };
    Transform.Translation = Loc;
    Transform.Rotation = FRotToQuat(Rot);
    return (T*)SpawnActor(Class, Transform, Owner);
}
template<typename T>
T* SpawnActor(UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
{
    FTransform Transform{};
    Transform.Scale3D = FVector(1, 1, 1);
    Transform.Translation = Loc;
    Transform.Rotation = FRotToQuat(Rot);
    return (T*)SpawnActor(Class, Transform, Owner);
}

UFortEngine* GetEngine()
{
	static auto Engine = (UFortEngine*)UEngine::GetEngine();
	return Engine;
}

UWorld* GetWorld()
{
	return UWorld::GetWorld();
}

bool IsInLobby() {
	if (auto World = SDK::UWorld::GetWorld())
	{
		if (World->GetName().contains("Frontend"))
		{
			return true;
		}
	}
}

UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GetGamePhaseLogic() {
	UFortGameStateComponent_BattleRoyaleGamePhaseLogic* (__fastcall * GetGamePhaseLogicOG)(UObject*) = decltype(GetGamePhaseLogicOG)(InSDKUtils::GetImageBase() + 0x187FDCC);
	return GetGamePhaseLogicOG(GetWorld());
}
static inline void* _NpFH = nullptr;
template <typename _Ot = void*>
__forceinline static void ExecHook(UFunction* _Fn, void* _Detour, _Ot& _Orig = _NpFH)
{
    if (!_Fn)
        return;
    if (!std::is_same_v<_Ot, void*>)
        _Orig = (_Ot)_Fn->ExecFunction;

    _Fn->ExecFunction = reinterpret_cast<UFunction::FNativeFuncPtr>(_Detour);
}


template <typename _Ot = void*>
__forceinline static void ExecHook(std::string _Name, void* _Detour, _Ot& _Orig = _NpFH)
{
    UFunction* _Fn = StaticLoadObject<UFunction>(_Name);
    ExecHook(_Fn, _Detour, _Orig);
}

#endif // !UTILS


