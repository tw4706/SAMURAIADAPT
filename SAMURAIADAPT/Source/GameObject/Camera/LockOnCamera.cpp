#include "LockOnCamera.h"
#include "Enemy/EnemyBase.h"
#include "Player/Player.h"
#include "Game.h"
#include <cmath>

namespace
{
	//lerpに割合(カメラのターゲット用)
	constexpr float kCameraTargetLerpRate = 0.1f;

	//カメラの向きを更新する敵とプレイヤーの最小水平距離
	//(近すぎると少しの移動で角度が大きく変わりブレるため)
	constexpr float kMinDirLength = 50.0f;
	constexpr float kMinDirLengthSq = kMinDirLength * kMinDirLength;

	//Lerpの割合(カメラの水平角度の追従用)
	constexpr float kYawLerpRate = 0.1f;

	//ターゲットまでの距離
	constexpr float kTargetDistance = 300.0f;

	//ターゲット座標のオフセット
	const Vector3 kTargetPosOffest = { 0.0f,100.0f,50.0f };

	//Lerpの割合(ターゲット座標の追従用)
	constexpr float kTargetPosLerpRate = 0.3f;

	//壁に当たった時にカメラを壁から離す距離
	constexpr float kWallMargin = 20.0f;

	//各デバッグ描画の定数
	constexpr int kDebugCameraPosY = 100;
	constexpr int kDebugTargetPosY = 130;
	constexpr int kDebugdirectionVecPosY = 160;
}

LockOnCamera::LockOnCamera():
	lockOnYaw_(0.0f),
	isFirstUpdate_(true)
{
}

LockOnCamera::~LockOnCamera()
{
}

void LockOnCamera::Init()
{
	lockOnYaw_ = 0.0f;
	isFirstUpdate_ = true;

	//切り替え前のカメラの座標から補間を始める
	idealPos_ = pos_;
}

Vector3 LockOnCamera::CheckCollLockOnCamera(int stageModelHandle, const Vector3& startPos, const Vector3& endPos) const
{
	MV1_COLL_RESULT_POLY result = MV1CollCheck_Line(stageModelHandle, -1, startPos.ToDxlibVector(), endPos.ToDxlibVector());

	if (result.HitFlag != 1) return endPos;

	//壁の法線方向に押し出すと横に大きく飛ぶため、プレイヤーに向かう線上で壁の少し手前に置く
	Vector3 hitPos = result.HitPosition;
	Vector3 toStart = startPos - hitPos;

	if (toStart.LengthSq() <= kWallMargin * kWallMargin) return startPos;

	return hitPos + toStart.Normalize() * kWallMargin;
}

void LockOnCamera::Update(int stageModelHandle)
{
	auto player = pPlayer_.lock();
	auto enemy = pEnemy_.lock();

	//プレイヤーまたは敵が存在しない場合は何もしない
	if (!player || !enemy)return;

	//敵の座標を取得
	Vector3 enemyTargetPos = enemy->GetCameraTarget();

	//カメラのターゲット座標に代入
	cameraTarget_ = Vector3::Lerp(cameraTarget_,enemyTargetPos, kCameraTargetLerpRate);

	Vector3 playerCameraPos = player->GetCameraTarget();

	//敵からプレイヤーへ向かう水平方向のベクトルを作成
	Vector3 toPlayerVec = playerCameraPos - enemyTargetPos;
	toPlayerVec.y_ = 0.0f;

	//切り替え直後は前のカメラの位置から角度を求めて急に回り込まないようにする
	if (isFirstUpdate_)
	{
		Vector3 fromPlayerVec = pos_ - playerCameraPos;
		fromPlayerVec.y_ = 0.0f;

		if (fromPlayerVec.LengthSq() >= kMinDirLengthSq)
		{
			lockOnYaw_ = std::atan2f(fromPlayerVec.x_, fromPlayerVec.z_);
		}
		else if (toPlayerVec.LengthSq() >= kMinDirLengthSq)
		{
			lockOnYaw_ = std::atan2f(toPlayerVec.x_, toPlayerVec.z_);
		}
		isFirstUpdate_ = false;
	}

	//敵と近すぎるときは角度を更新しない
	if (toPlayerVec.LengthSq() >= kMinDirLengthSq)
	{
		float targetYaw = std::atan2f(toPlayerVec.x_, toPlayerVec.z_);

		//差分を-π～πに収めて遠回りしないようにする
		float diffYaw = std::remainderf(targetYaw - lockOnYaw_, DX_TWO_PI_F);

		//角度を補間して左右のブレを抑える
		lockOnYaw_ += diffYaw * kYawLerpRate;
	}

	Vector3 directionVec = { std::sinf(lockOnYaw_),0.0f,std::cosf(lockOnYaw_) };

	//ロックオン時のカメラのターゲット座標の計算
	Vector3 targetPos = playerCameraPos + (directionVec * kTargetDistance) + kTargetPosOffest;

	//線形補間を行い滑らかな追従を行う
	//(壁で押し戻した座標から補間すると壁際でガタつくため、押し戻す前の座標で補間する)
	idealPos_ = Vector3::Lerp(idealPos_, targetPos, kTargetPosLerpRate);

	//カメラとステージの当たり判定を行う
	pos_ = CheckCollLockOnCamera(stageModelHandle, playerCameraPos, idealPos_);

	//デバッグ描画
#ifdef _DEBUG
	DrawFormatString(0, kDebugCameraPosY, Game::kRedColor, L"CameraPos: %.2f, %.2f, %.2f", pos_.x_, pos_.y_, pos_.z_);
	DrawFormatString(0, kDebugTargetPosY, Game::kYellowColor, L"TargetPos: %.2f, %.2f, %.2f", targetPos.x_, targetPos.y_, targetPos.z_);
	DrawFormatString(0, kDebugdirectionVecPosY, Game::kGreenColor, L"DirVec: %.2f, %.2f, %.2f", directionVec.x_, directionVec.y_, directionVec.z_);
#endif

	//基底クラスの更新
	CameraBase::Update(stageModelHandle);
}
