#pragma once
namespace Time
{

#define MICRO2SEC(micro_sec) micro_sec*0.000001		//!< マイクロ秒 -> 秒への変換マクロ
#define SEC2MICRO(sec) sec*1000000					//!< 秒 -> マイクロ秒への変換マクロ
#define MILLI2SEC(micro_sec) micro_sec*0.001		//!< ミリ秒 -> 秒への変換マクロ 
#define SEC2MILLI(sec) sec*1000						//!< 秒 -> ミリ秒への変換マクロ


	//初期化
	int Init();
	//更新
	void Update();
	//FPS更新
	void UpdateFPS();


	//時飛ばし(時間の上書きなのであまり多用しないでください)
	void ResetTime();


	//処理を指定FPSに固定
	int FixFPS();
	//描画処理の経過時間を初期化・描画FPSを計測
	void FixDrawFPS();
	//物理処理の経過時間を更新・物理処理のFPSを計測
	void FixFixedFPS();


	//タイムスケールの取得(float)
	const float TimeScale();
	//タイムスケールの取得(double)
	const double TimeScaleD();

	//タイムスケールの変更
	void SetTimeScale(const double scale);


	//ゲーム内Δ時間の取得(float)
	const float DeltaTime();
	//ゲーム内Δ時間の取得(double)
	const double DeltaTimeD();
	//描画用Δ時間の取得(float)
	const float DrawDeltaTime();
	//描画用Δ時間の取得(double)
	const double DrawDeltaTimeD();
	//物理用Δ時間の取得(float)
	const float FixedDeltaTime();
	//物理用Δ時間の取得(double)
	const double FixedDeltaTimeD();
	//前フレームとの物理的時間差の取得(float)
	const float UnscaledDeltaTime();
	//前フレームとの物理的時間差の取得(double)
	const double UnscaledDeltaTimeD();
	//前フレームとの実際の物理的時間差の取得(float)
	const float RealDeltaTime();
	//前フレームとの実際の物理的時間差の取得(double)
	const double RealDeltaTimeD();


	//アプリケーション開始後の物理的時間の取得(float)
	const float SystemTimeFromStart();
	//アプリケーション開始後の物理的時間の取得(double)
	const double SystemTimeFromStartD();
	//アプリケーション開始後のゲーム内時間の取得(float)
	const float GetTimeFromStart();
	//アプリケーション開始後のゲーム内時間の取得(double)
	const double GetTimeFromStartD();
	//アプリケーション開始後の現実時間の取得(float)
	const float GetRealTimeFromStart();
	//アプリケーション開始後の現実時間の取得(double)
	const double GetRealTimeFromStartD();

	//システム内時間の取得(float)
	const float SystemTime();
	//システム内時間の取得(double)
	const double SystemTimeD();
	//Windowsから取得した現在時刻(float)
	const float GetOSTime();
	//Windowsから取得した現在時刻(double)
	const double GetOSTimeD();


	//FPSの最大値を取得(float)
	const float GetFPSMAX();
	//FPSの最大値を取得(double)
	const double GetFPSMAXD();
	//描画FPSの最大値を取得(float)
	const float GetDrawFPSMAX();
	//描画FPSの最大値を取得(double)
	const double GetDrawFPSMAXD();

	//FPSの最大値を設定
	void SetFPSMAX(const double& max);
	//物理更新FPSの最大値を設定
	void SetFixedFPSMAX(const double& max);
	//描画FPSの最大値を設定
	void SetDrawFPSMAX(const double& max);


	//物理Δ時間の最大値取得(float)
	const float GetFixedDeltaTimeMAX();
	//物理Δ時間の最大値取得(double)
	const double GetFixedDeltaTimeMAXD();
	//描画Δ時間の最大値取得(float)
	const float GetDrawDeltaTimeMAX();
	//描画Δ時間の最大値取得(double)
	const double GetDrawDeltaTimeMAXD();
	//Δ時間の最大値取得(float)
	const float GetDeltaTimeMAX();
	//Δ時間の最大値取得(double)
	const double GetDeltaTimeMAXD();


	//FPSの取得(float)
	const float GetFPS();
	//FPSの取得(double)
	const double GetFPSD();
};

