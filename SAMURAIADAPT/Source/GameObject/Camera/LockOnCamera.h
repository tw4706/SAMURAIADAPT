#pragma once
#include "CameraBase.h"

class Player;
class EnemyBase;
class LockOnCamera :public CameraBase
{
public:
	LockOnCamera();
	virtual~LockOnCamera();

	virtual void Init()override;
	virtual void Update(int stageModelHandle)override;

	void SetPlayer(std::weak_ptr<Player>pPlayer) { pPlayer_ = pPlayer; }
	void SetTargetEnemy(std::weak_ptr<EnemyBase>pEnemy) { pEnemy_ = pEnemy; }

private:
	/// <summary>
	/// カメラとステージの当たり判定(壁の手前にカメラを置く)
	/// </summary>
	/// <param name="stageModelHandle">ステージのモデルハンドル</param>
	/// <param name="startPos">始点(プレイヤーの注視点)</param>
	/// <param name="endPos">終点(押し戻す前のカメラ座標)</param>
	/// <returns>壁を考慮したカメラ座標</returns>
	Vector3 CheckCollLockOnCamera(int stageModelHandle, const Vector3& startPos, const Vector3& endPos) const;

	Vector3 idealPos_;		//壁で押し戻す前のカメラ座標
	float lockOnYaw_;		//プレイヤーから見たカメラの水平角度(補間して左右のブレを抑える)
	bool isFirstUpdate_;	//切り替え直後の最初の更新かどうか

	std::weak_ptr<Player>pPlayer_;//プレイヤーのポインタ
	std::weak_ptr<EnemyBase>pEnemy_;//敵のポインタ
};

