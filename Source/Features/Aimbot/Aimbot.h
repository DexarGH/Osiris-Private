#pragma once

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <CS2/Classes/CGameSceneNode.h>
#include <CS2/Classes/EntitySystem/CEntityClass.h>
#include <CS2/Classes/EntitySystem/CEntityIdentity.h>
#include <CS2/Classes/Entities/C_CSPlayerPawn.h>
#include <GameClient/Entities/BaseEntity.h>
#include <GameClient/Entities/PlayerPawn.h>
#include <GameClient/EntitySystem/EntitySystem.h>
#include <GameClient/WorldToScreen/WorldToClipSpaceConverter.h>
#include <Platform/Macros/IsPlatform.h>
#if IS_WIN64()
#include <Platform/Windows/WindowsDynamicLibrary.h>
#endif
#include <SDL/SdlDll.h>

#if IS_LINUX()
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#endif

#include "AimbotActivationKeys.h"
#include "AimbotConfigVariables.h"
#include "AimbotParams.h"
#include "AimbotState.h"

template <typename HookContext>
class Aimbot {
public:
    explicit Aimbot(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void run() noexcept
    {
#if !IS_LINUX() && !IS_WIN64()
        return;
#else
        if (!enabled() || !isActivationBindingPressed())
            return;

        // Проверяем ослепление если включено
        if (flashChecks() && isPlayerBlinded())
            return;

        if (const auto target = findTargetInNdc(); target.has_value())
            moveMouseTowards(*target);
#endif
    }

private:
    struct TargetInNdc {
        float x;
        float y;
        float ndcDistanceSquared;
        float selectionDistanceSquared;
    };

    struct ProjectedAimPoint {
        float x;
        float y;
        float ndcDistanceSquared;
        cs2::Vector worldPosition;
    };

    struct BoneReadLayout {
        std::uint8_t boneStride;
        std::uint8_t positionOffset;
    };

    struct BonePair {
        std::uint8_t headBoneIndex;
        std::uint8_t bodyBoneIndex;
    };

    struct SelectedAimBones {
        cs2::Vector head;
        cs2::Vector body;
    };

    [[nodiscard]] bool enabled() const noexcept
    {
        return hookContext.config().template getVariable<aimbot::Enabled>();
    }

    [[nodiscard]] std::size_t bindIndex() const noexcept
    {
        return static_cast<std::size_t>(static_cast<std::uint8_t>(hookContext.config().template getVariable<aimbot::Bind>()));
    }

    [[nodiscard]] aimbot::BindMode bindMode() const noexcept
    {
        return hookContext.config().template getVariable<aimbot::BindModeType>();
    }

    [[nodiscard]] float smooth() const noexcept
    {
        return static_cast<float>(static_cast<std::uint16_t>(hookContext.config().template getVariable<aimbot::Smooth>()));
    }

    [[nodiscard]] aimbot::RotationType rotationType() const noexcept
    {
        return hookContext.config().template getVariable<aimbot::Rotation>();
    }

    [[nodiscard]] float fov() const noexcept
    {
        return static_cast<float>(static_cast<std::uint16_t>(hookContext.config().template getVariable<aimbot::Fov>()));
    }

    [[nodiscard]] float multiPointSize() const noexcept
    {
        return aimbot_params::multiPointSizeFromSlider(static_cast<std::uint16_t>(hookContext.config().template getVariable<aimbot::MultiPointSize>()));
    }

    [[nodiscard]] bool visibleChecks() const noexcept
    {
        return hookContext.config().template getVariable<aimbot::VisibleChecks>();
    }

    [[nodiscard]] bool flashChecks() const noexcept
    {
        return hookContext.config().template getVariable<aimbot::FlashChecks>();
    }

    [[nodiscard]] bool isActivationBindingPressed() noexcept
    {
        const auto currentBindMode = bindMode();
        
        // Always On - всегда активен если включен
        if (currentBindMode == aimbot::BindMode::AlwaysOn)
            return true;
        
        // Hold или Toggle - нужно проверять нажатие кнопки
        const auto bindingIdx = bindIndex();
        if (bindingIdx >= aimbot::kActivationKeyBindings.size())
            return false;

        const auto binding = aimbot::kActivationKeyBindings[bindingIdx];
        bool isPressed = false;
        if (binding.inputType == aimbot::ActivationInputType::Keyboard)
            isPressed = isKeyboardKeyPressed(binding.code);
        else
            isPressed = isMouseButtonPressed(static_cast<std::uint8_t>(binding.code));
        
        // Toggle mode - переключение состояния
        if (currentBindMode == aimbot::BindMode::Toggle) {
            if (isPressed && !state().lastBindWasPressed) {
                state().toggleState = !state().toggleState;
            }
            state().lastBindWasPressed = isPressed;
            return state().toggleState;
        }
        
        // Hold mode - работает только пока зажата кнопка
        return isPressed;
    }

    [[nodiscard]] bool isKeyboardKeyPressed(std::size_t scancode) noexcept
    {
        auto& getKeyboardState = state().getKeyboardState;
        if (!getKeyboardState)
            getKeyboardState = SdlDll{}.getKeyboardState();
        if (!getKeyboardState)
            return false;

        int numKeys = 0;
        const auto keyboardState = getKeyboardState(&numKeys);
        if (!keyboardState)
            return false;

        if (numKeys <= 0 || static_cast<std::size_t>(numKeys) <= scancode)
            return false;
        return keyboardState[scancode] != 0;
    }

    [[nodiscard]] bool isMouseButtonPressed(std::uint8_t button) noexcept
    {
        auto& getMouseState = state().getMouseState;
        if (!getMouseState)
            getMouseState = SdlDll{}.getMouseState();
        if (!getMouseState)
            return false;

        const auto mouseButtons = getMouseState(nullptr, nullptr);
        return (mouseButtons & aimbot::mouseButtonMask(button)) != 0;
    }

    [[nodiscard]] std::optional<TargetInNdc> findTargetInNdc() const noexcept
    {
        std::optional<TargetInNdc> nearestTarget;
        WorldToClipSpaceConverter<HookContext> worldToClipSpaceConverter{hookContext};
        std::uint16_t inspectedEntities = 0;
        std::uint16_t enemyPawns = 0;
        std::uint16_t validProjectedPoints = 0;

        hookContext.template make<EntitySystem>().forEachNetworkableEntityIdentity([&](const cs2::CEntityIdentity& entityIdentity) {
            ++inspectedEntities;
            if (!isEnemyPlayerPawn(entityIdentity.entity, entityIdentity.entityClass))
                return;
            ++enemyPawns;

            auto&& playerPawn = hookContext.template make<BaseEntity>(static_cast<cs2::C_BaseEntity*>(entityIdentity.entity)).template as<PlayerPawn>();
            const auto projectedAimPoint = projectAimPointFromBones(playerPawn, worldToClipSpaceConverter);
            if (!projectedAimPoint.has_value())
                return;
            ++validProjectedPoints;

            const auto selectionDistanceSquared = projectedAimPoint->ndcDistanceSquared;

            if (!nearestTarget.has_value() || selectionDistanceSquared < nearestTarget->selectionDistanceSquared) {
                nearestTarget = TargetInNdc{
                    .x = projectedAimPoint->x,
                    .y = projectedAimPoint->y,
                    .ndcDistanceSquared = projectedAimPoint->ndcDistanceSquared,
                    .selectionDistanceSquared = selectionDistanceSquared
                };
            }
        });

        maybeWriteDebugLog(inspectedEntities, enemyPawns, validProjectedPoints, nearestTarget.has_value());
        return nearestTarget;
    }

    [[nodiscard]] std::optional<ProjectedAimPoint> projectAimPointFromBones(const PlayerPawn<HookContext>& playerPawn, const WorldToClipSpaceConverter<HookContext>& converter) const noexcept
    {
        const auto selectedAimBones = selectAimBones(playerPawn);
        if (!selectedAimBones.has_value())
            return std::nullopt;

        // Используем head по умолчанию
        const auto& aimWorldPosition = selectedAimBones->head;

        const auto clip = converter.toClipSpace(aimWorldPosition);
        if (!clip.onScreen())
            return std::nullopt;

        const auto inverseW = 1.0f / clip.w;
        const auto ndcX = clip.x * inverseW;
        const auto ndcY = clip.y * inverseW;
        const auto ndcDistanceSquared = ndcX * ndcX + ndcY * ndcY;
        
        // Проверяем FOV
        const auto fovRad = fov() * (3.14159265f / 180.0f);
        const auto fovLimit = std::tan(fovRad * 0.5f);
        if (ndcDistanceSquared > fovLimit * fovLimit)
            return std::nullopt;
        
        // Проверяем видимость если включено
        if (visibleChecks() && !isTargetVisible(aimWorldPosition))
            return std::nullopt;

        return ProjectedAimPoint{
            .x = ndcX,
            .y = ndcY,
            .ndcDistanceSquared = ndcDistanceSquared,
            .worldPosition = aimWorldPosition
        };
    }

    [[nodiscard]] bool isTargetVisible(const cs2::Vector& targetPosition) const noexcept
    {
        // TODO: Requires engine trace function pattern (IEngineTrace::TraceRay or UTIL_TraceLine)
        // Need pattern scan for Linux CS2 to find trace function address
        // Without it, cannot implement proper visibility checks
        static_cast<void>(targetPosition);
        return true;
    }

    [[nodiscard]] bool isPlayerBlinded() const noexcept
    {
        // Проверяем ослепление локального игрока
        auto&& localPlayer = hookContext.activeLocalPlayerPawn();
        if (!localPlayer)
            return false;
        
        // Если оставшееся время ослепления > 0 — игрок ослеплён
        return localPlayer.getRemainingFlashBangTime() > 0.0f;
    }

    [[nodiscard]] std::optional<SelectedAimBones> selectAimBones(const PlayerPawn<HookContext>& playerPawn) const noexcept
    {
        const auto gameSceneNode = static_cast<cs2::CGameSceneNode*>(playerPawn.baseEntity().gameSceneNode());
        if (!gameSceneNode)
            return std::nullopt;

        std::optional<cs2::Vector> playerOrigin;
        if (const auto absOrigin = playerPawn.absOrigin(); absOrigin.hasValue())
            playerOrigin = absOrigin.value();

        auto& aimbotState = state();
        std::optional<SelectedAimBones> selectedAimBones;
        float bestScore = -1.0f;
        std::uintptr_t bestPointerFieldOffset = 0;

        const auto evaluateBoneArrayPointerFieldOffset = [&](std::uintptr_t pointerFieldOffset, float scoreBonus) {
            for (const auto& readLayout : kBoneReadLayouts) {
                for (const auto& bonePair : kPreferredBonePairs) {
                    const auto headPosition = boneWorldPosition(gameSceneNode, pointerFieldOffset, readLayout, bonePair.headBoneIndex);
                    if (!headPosition.has_value())
                        continue;

                    const auto bodyPosition = boneWorldPosition(gameSceneNode, pointerFieldOffset, readLayout, bonePair.bodyBoneIndex);
                    if (!bodyPosition.has_value())
                        continue;

                    auto score = scoreBonePair(*headPosition, *bodyPosition, playerOrigin) + scoreBonus;

                    if (score > bestScore) {
                        bestScore = score;
                        bestPointerFieldOffset = pointerFieldOffset;
                        selectedAimBones = SelectedAimBones{
                            .head = *headPosition,
                            .body = *bodyPosition
                        };
                    }
                }
            }
        };

        if (aimbotState.hasResolvedBoneArrayPointerFieldOffset)
            evaluateBoneArrayPointerFieldOffset(aimbotState.resolvedBoneArrayPointerFieldOffset, kResolvedPointerFieldOffsetScoreBonus);

        if (bestScore < kMinBonePairScore) {
            if (aimbotState.boneArrayPointerFieldRescanCooldown > 0) {
                --aimbotState.boneArrayPointerFieldRescanCooldown;
                evaluateBoneArrayPointerFieldOffset(kPreferredBoneArrayPointerFieldOffset, kPreferredPointerFieldOffsetScoreBonus);
            } else {
                evaluateBoneArrayPointerFieldOffset(kPreferredBoneArrayPointerFieldOffset, kPreferredPointerFieldOffsetScoreBonus);
                for (std::uintptr_t pointerFieldOffset = kBoneArrayPointerFieldOffsetSearchMin; pointerFieldOffset <= kBoneArrayPointerFieldOffsetSearchMax; pointerFieldOffset += kBoneArrayPointerFieldOffsetSearchStep) {
                    if (pointerFieldOffset == kPreferredBoneArrayPointerFieldOffset)
                        continue;
                    evaluateBoneArrayPointerFieldOffset(pointerFieldOffset, 0.0f);
                }
            }
        }

        if (bestScore < kMinBonePairScore) {
            aimbotState.hasResolvedBoneArrayPointerFieldOffset = false;
            aimbotState.boneArrayPointerFieldRescanCooldown = kBoneArrayPointerFieldRescanCooldownFrames;
            return std::nullopt;
        }

        aimbotState.hasResolvedBoneArrayPointerFieldOffset = true;
        aimbotState.resolvedBoneArrayPointerFieldOffset = static_cast<std::uint16_t>(bestPointerFieldOffset);
        aimbotState.boneArrayPointerFieldRescanCooldown = 0;
        return selectedAimBones;
    }

    [[nodiscard]] std::optional<cs2::Vector> boneWorldPosition(const cs2::CGameSceneNode* gameSceneNode, std::uintptr_t boneArrayPointerFieldOffset, BoneReadLayout readLayout, std::uint8_t boneIndex) const noexcept
    {
        const auto gameSceneNodeAddress = reinterpret_cast<std::uintptr_t>(gameSceneNode);
        std::uintptr_t boneArrayAddress{};
        if (!safeRead(gameSceneNodeAddress + boneArrayPointerFieldOffset, boneArrayAddress))
            return std::nullopt;
        if (!isLikelyUserSpaceAddress(boneArrayAddress))
            return std::nullopt;

        const auto boneAddress = boneArrayAddress + static_cast<std::uintptr_t>(boneIndex) * readLayout.boneStride + readLayout.positionOffset;
        if (!isLikelyUserSpaceAddress(boneAddress))
            return std::nullopt;

        cs2::Vector bonePosition{};
        if (!safeRead(boneAddress, bonePosition))
            return std::nullopt;
        if (!isFiniteVector(bonePosition))
            return std::nullopt;

        if (std::abs(bonePosition.x) > kMaxWorldCoordinateAbsValue || std::abs(bonePosition.y) > kMaxWorldCoordinateAbsValue || std::abs(bonePosition.z) > kMaxWorldCoordinateAbsValue)
            return std::nullopt;

        return bonePosition;
    }

    [[nodiscard]] static float scoreBonePair(const cs2::Vector& headPosition, const cs2::Vector& bodyPosition, const std::optional<cs2::Vector>& playerOrigin) noexcept
    {
        const auto verticalDistance = headPosition.z - bodyPosition.z;
        if (verticalDistance < kMinHeadToBodyHeight || verticalDistance > kMaxHeadToBodyHeight)
            return -1.0f;

        const auto horizontalDistanceSquared = distanceSquared2D(headPosition, bodyPosition);
        if (horizontalDistanceSquared > kMaxHeadToBodyHorizontalDistanceSquared)
            return -1.0f;

        auto score = 200.0f - std::abs(verticalDistance - kIdealHeadToBodyHeight) * 8.0f - std::sqrt(horizontalDistanceSquared) * 4.0f;

        if (playerOrigin.has_value()) {
            const auto bodyHeightFromOrigin = bodyPosition.z - playerOrigin->z;
            const auto headHeightFromOrigin = headPosition.z - playerOrigin->z;
            if (bodyHeightFromOrigin < kMinBodyHeightFromOrigin || bodyHeightFromOrigin > kMaxBodyHeightFromOrigin)
                return -1.0f;
            if (headHeightFromOrigin < kMinHeadHeightFromOrigin || headHeightFromOrigin > kMaxHeadHeightFromOrigin)
                return -1.0f;

            const auto bodyOriginDistanceSquared = distanceSquared2D(bodyPosition, *playerOrigin);
            const auto headOriginDistanceSquared = distanceSquared2D(headPosition, *playerOrigin);
            if (bodyOriginDistanceSquared > kMaxBodyOriginHorizontalDistanceSquared || headOriginDistanceSquared > kMaxHeadOriginHorizontalDistanceSquared)
                return -1.0f;

            score -= std::abs(bodyHeightFromOrigin - kIdealBodyHeightFromOrigin) * 0.5f;
            score -= std::abs(headHeightFromOrigin - kIdealHeadHeightFromOrigin) * 0.4f;
            score -= std::sqrt(bodyOriginDistanceSquared) * 0.4f;
            score -= std::sqrt(headOriginDistanceSquared) * 0.4f;
        }

        return score;
    }

    [[nodiscard]] static bool isLikelyUserSpaceAddress(std::uintptr_t address) noexcept
    {
        return address >= 0x10000 && address < 0x0000800000000000;
    }

    template <typename T>
    [[nodiscard]] bool safeRead(std::uintptr_t address, T& value) const noexcept
    {
#if IS_LINUX()
        if (!isLikelyUserSpaceAddress(address))
            return false;

        if (!state().processVmReadvUnavailable) {
            iovec localIov{
                .iov_base = &value,
                .iov_len = sizeof(T)
            };
            iovec remoteIov{
                .iov_base = reinterpret_cast<void*>(address),
                .iov_len = sizeof(T)
            };
            if (::process_vm_readv(::getpid(), &localIov, 1, &remoteIov, 1, 0) == static_cast<ssize_t>(sizeof(T)))
                return true;
            if (errno == EPERM || errno == ENOSYS)
                state().processVmReadvUnavailable = true;
        }

        if (safeReadFromProcMem(address, &value, sizeof(T)))
            return true;

        return false;
#elif IS_WIN64()
        if (!isLikelyUserSpaceAddress(address))
            return false;

        auto& readProcessMemory = state().readProcessMemory;
        if (!readProcessMemory)
            readProcessMemory = WindowsDynamicLibrary{"kernel32.dll"}.getFunctionAddress("ReadProcessMemory").as<decltype(&::ReadProcessMemory)>();
        if (!readProcessMemory)
            return false;

        SIZE_T bytesRead = 0;
        return readProcessMemory(reinterpret_cast<HANDLE>(-1), reinterpret_cast<const void*>(address), &value, sizeof(T), &bytesRead) && bytesRead == sizeof(T);
#else
        static_cast<void>(address);
        static_cast<void>(value);
        return false;
#endif
    }

    [[nodiscard]] bool safeReadFromProcMem(std::uintptr_t address, void* destination, std::size_t size) const noexcept
    {
#if IS_LINUX()
        auto& memFd = state().selfMemFd;
        if (memFd == -2)
            return false;
        if (memFd < 0) {
            memFd = ::open("/proc/self/mem", O_RDONLY | O_CLOEXEC);
            if (memFd < 0) {
                memFd = -2;
                return false;
            }
        }

        return ::pread(memFd, destination, size, static_cast<off_t>(address)) == static_cast<ssize_t>(size);
#else
        static_cast<void>(address);
        static_cast<void>(destination);
        static_cast<void>(size);
        return false;
#endif
    }

    [[nodiscard]] static bool isFiniteVector(const cs2::Vector& vector) noexcept
    {
        return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z);
    }

    [[nodiscard]] static float distanceSquared(const cs2::Vector& from, const cs2::Vector& to) noexcept
    {
        const auto dx = to.x - from.x;
        const auto dy = to.y - from.y;
        const auto dz = to.z - from.z;
        return dx * dx + dy * dy + dz * dz;
    }

    [[nodiscard]] static float distanceSquared2D(const cs2::Vector& from, const cs2::Vector& to) noexcept
    {
        const auto dx = to.x - from.x;
        const auto dy = to.y - from.y;
        return dx * dx + dy * dy;
    }

    [[nodiscard]] bool isEnemyPlayerPawn(cs2::CEntityInstance* entity, const cs2::CEntityClass* entityClass) const noexcept
    {
        const auto entityTypeInfo = hookContext.entityClassifier().classifyEntity(entityClass);
        if (!entityTypeInfo.template is<cs2::C_CSPlayerPawn>())
            return false;

        auto&& playerPawn = hookContext.template make<BaseEntity>(static_cast<cs2::C_BaseEntity*>(entity)).template as<PlayerPawn>();
        return playerPawn
            && !playerPawn.isControlledByLocalPlayer()
            && playerPawn.isAlive().value_or(false)
            && playerPawn.isEnemy().value_or(false);
    }

    void moveMouseTowards(const TargetInNdc& target) noexcept
    {
        const auto smoothValue = smooth();
        const auto rotation = rotationType();
        
        // Вычисляем движение
        float moveX = target.x;
        float moveY = -target.y;
        
        // Применяем сглаживание и тип ротации
        if (rotation == aimbot::RotationType::Sigmoid) {
            // Sigmoid функция для плавности
            const auto distance = std::sqrt(moveX * moveX + moveY * moveY);
            if (distance > 0.001f) {
                const auto speed = 1000.0f / (smoothValue * 10.0f + 1.0f);
                const auto sigmoidFactor = 1.0f / (1.0f + expf(-4.0f * (distance / (smoothValue + 1.0f) - 0.5f)));
                moveX *= speed * sigmoidFactor;
                moveY *= speed * sigmoidFactor;
            }
        } else {
            // Linear smooth: при smooth=0 -> factor=100 (быстро), при smooth=20 -> factor~4.7
            const auto speedFactor = 100.0f / (smoothValue + 1.0f);
            moveX *= speedFactor;
            moveY *= speedFactor;
        }

        moveMouseRelative(moveX, moveY);
    }

    [[nodiscard]] auto& state() const noexcept
    {
        return hookContext.featuresStates().aimbotState;
    }

    void moveMouseRelative(float deltaX, float deltaY) const noexcept
    {
#if IS_LINUX()
        state().linuxInput.moveMouseRelative(deltaX, deltaY);
#elif IS_WIN64()
        state().windowsInput.moveMouseRelative(deltaX, deltaY);
#else
        static_cast<void>(deltaX);
        static_cast<void>(deltaY);
#endif
    }

    void maybeWriteDebugLog(std::uint16_t inspectedEntities, std::uint16_t enemyPawns, std::uint16_t validProjectedPoints, bool hasTarget) const noexcept
    {
#if IS_LINUX()
        auto& aimbotState = state();
        ++aimbotState.debugLogCounter;
        if (aimbotState.debugLogCounter % kDebugLogSampleInterval != 0)
            return;

        if (aimbotState.debugLogFd < 0) {
            aimbotState.debugLogFd = ::open("/tmp/osiris_aimbot_debug.log", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
            if (aimbotState.debugLogFd < 0)
                return;
        }

        char logLine[256];
        const auto lineLength = std::snprintf(
            logLine,
            sizeof(logLine),
            "inspected=%u enemy=%u projected=%u target=%u pvreadv_unavail=%u memfd=%d cached_offset=%u has_cached_offset=%u rescan_cooldown=%u\n",
            static_cast<unsigned>(inspectedEntities),
            static_cast<unsigned>(enemyPawns),
            static_cast<unsigned>(validProjectedPoints),
            hasTarget ? 1u : 0u,
            aimbotState.processVmReadvUnavailable ? 1u : 0u,
            aimbotState.selfMemFd,
            static_cast<unsigned>(aimbotState.resolvedBoneArrayPointerFieldOffset),
            aimbotState.hasResolvedBoneArrayPointerFieldOffset ? 1u : 0u,
            static_cast<unsigned>(aimbotState.boneArrayPointerFieldRescanCooldown));
        if (lineLength > 0)
            ::write(aimbotState.debugLogFd, logLine, static_cast<std::size_t>(lineLength));
#else
        static_cast<void>(inspectedEntities);
        static_cast<void>(enemyPawns);
        static_cast<void>(validProjectedPoints);
        static_cast<void>(hasTarget);
#endif
    }

    static constexpr std::array<BoneReadLayout, 3> kBoneReadLayouts{ {
        {.boneStride = 32, .positionOffset = 0},
        {.boneStride = 32, .positionOffset = 16},
        {.boneStride = 48, .positionOffset = 12}
    } };

    static constexpr std::array<BonePair, 6> kPreferredBonePairs{ {
        {.headBoneIndex = 7, .bodyBoneIndex = 5},
        {.headBoneIndex = 7, .bodyBoneIndex = 4},
        {.headBoneIndex = 6, .bodyBoneIndex = 5},
        {.headBoneIndex = 6, .bodyBoneIndex = 4},
        {.headBoneIndex = 8, .bodyBoneIndex = 5},
        {.headBoneIndex = 8, .bodyBoneIndex = 6}
    } };

    static constexpr float kIdealHeadToBodyHeight{18.0f};
    static constexpr float kMinHeadToBodyHeight{2.0f};
    static constexpr float kMaxHeadToBodyHeight{60.0f};
    static constexpr float kMaxHeadToBodyHorizontalDistanceSquared{900.0f};
    static constexpr float kIdealBodyHeightFromOrigin{40.0f};
    static constexpr float kIdealHeadHeightFromOrigin{66.0f};
    static constexpr float kMinBodyHeightFromOrigin{-30.0f};
    static constexpr float kMaxBodyHeightFromOrigin{140.0f};
    static constexpr float kMinHeadHeightFromOrigin{-10.0f};
    static constexpr float kMaxHeadHeightFromOrigin{220.0f};
    static constexpr float kMaxBodyOriginHorizontalDistanceSquared{14400.0f};
    static constexpr float kMaxHeadOriginHorizontalDistanceSquared{19600.0f};
    static constexpr float kMinBonePairScore{0.0f};
    static constexpr float kMaxWorldCoordinateAbsValue{65536.0f};

    static constexpr std::uintptr_t kPreferredBoneArrayPointerFieldOffset = 0x1F0;
    static constexpr float kPreferredPointerFieldOffsetScoreBonus{8.0f};
    static constexpr float kResolvedPointerFieldOffsetScoreBonus{16.0f};
    static constexpr std::uintptr_t kBoneArrayPointerFieldOffsetSearchMin = 0x140;
    static constexpr std::uintptr_t kBoneArrayPointerFieldOffsetSearchMax = 0x280;
    static constexpr std::uintptr_t kBoneArrayPointerFieldOffsetSearchStep = 0x8;
    static constexpr std::uint8_t kBoneArrayPointerFieldRescanCooldownFrames = 60;
    static constexpr std::uint8_t kDebugLogSampleInterval = 20;

    HookContext& hookContext;
};
