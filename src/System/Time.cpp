#include "precompile.h"
#include "Time.h"

namespace Time {
	//システム用の時間管理用変数(ulonglong管理)
	unsigned long long sys_time; //!<システム上の時間(現実世界の時間を参照)
	unsigned long long sys_time_start; //!<システム上のアプリケーション開始時間
	unsigned long long sys_time_prev; //!<システム上の前フレームの時間
	unsigned long long real_sys_time_prev; //!<正しいシステム上の時間(現実世界の時間を参照)

	//外部で使用するための時間管理用変数(double[秒]管理)
	double time;//!<アプリケーション開始後の経過時間(ゲーム内時間)
	double real_time;//!<アプリケーション開始後の経過時間(現実時間)
	double delta_time; //!<前フレームとの時間差 (Δ)
	double real_delta_time; //!<前フレームとの時間差 (Δ)
	double delta_time_max; //!<前フレームとの時間差の最大値
	double draw_delta_time; //<前描画フレームとの時間差 (Δ)
	double draw_delta_time_max; //!<前描画フレームとの時間差の最大値
	double fixed_delta_time; //!<前描画フレームとの時間差 (Δ)
	double fixed_delta_time_max; //!<前描画フレームとの時間差の最大値
	double time_scale = 1; //!<タイムスケール(ゲーム内時間の進行スピード)

	double fps_max = 60;		//!< UpdateFPS(内部更新)の最大値
	double draw_fps_max = 60;	//!< DrawFPS(描画)の最大値
	double fixed_fps_max = 50;	//!< PhysicsFPS(物理演算)の最大値
	double fps = 0;				//!< FPS(内部更新)の頻度
	double draw_fps = 0;		//!< DrawFPS(描画)の頻度
	double fixed_fps = 0;		//!< PhysicsFPS(物理演算)の頻度

	//初期化
	int Init()
	{
		sys_time = (unsigned long long)SEC2MICRO(GetOSTimeD());		//!<システム時間を初期化
		sys_time_start = sys_time;									//!<アプリケーション開始時刻を記録
		sys_time_prev = sys_time;									//!<前フレームのシステム時間を現在のものとして記録
		real_sys_time_prev = sys_time;								//!<正規の前フレーム時間を現在のものとして記録
		delta_time = 0;												//!<前フレームからのΔ時間を初期化
		real_delta_time = 0;												//!<前フレームからのΔ時間を初期化

		time = 0;										//!<アプリケーション開始後の時間を初期化
		real_time = 0;									//!<アプリケーション開始後の正規時間を初期化
		delta_time_max = 1.0 / fps_max;					//!<更新FPSの設定
		draw_delta_time_max = 1.0 / draw_fps_max;		//!<描画FPSの設定
		fixed_delta_time_max = 1.0 / fixed_fps_max;		//!<物理FPSの初期化
		return 0;
	}

	//更新
	void Update()
	{
		sys_time = (unsigned long long)(SEC2MICRO(GetOSTimeD()));					//!<システム時間を記録
		delta_time = (double)(MICRO2SEC((sys_time - sys_time_prev)));				//!<前フレームからの経過時間を測定
		real_delta_time = (double)(MICRO2SEC((sys_time - real_sys_time_prev)));		//!<正規の経過時間を記録
		real_time += real_delta_time;													//!<正規のアプリケーション時間を加算
		time += delta_time * time_scale;											//!<ゲーム内時間を加算
		draw_delta_time += delta_time;												//!<前回の描画からの経過時間を記録
		fixed_delta_time += delta_time;												//!<前回の物理更新からの経過時間を記録

		sys_time_prev = sys_time;						//!<前フレームのシステム時間を記録
		real_sys_time_prev = sys_time;					//!<前フレームの正規システム時間を記録
	}

	//FPS更新
	void UpdateFPS()
	{
		//!<ここで普通のdelta_timeを使用すると、時飛ばしを行った場合にFPSの計算が非正規のものになってしまう
		fps = 1.0 / real_delta_time;		//!<実際の経過時間を使用してFPSを測定

	}

	//処理を指定FPSに固定
	int FixFPS()
	{
		// FPS固定にはwhileを使用しない
		// whileでやると、FPSのリソースを馬鹿みたいに食うので、
		// Chronoを使用したSleep関数でアプリを一時停止する

		double now_time = Time::GetOSTimeD();														//!<現在の正規時間を取得
		double system_time = Time::SystemTimeD();													//!<現在のシステム時間を取得
		double sleep_time = delta_time_max - (now_time - system_time);								//!<Sleep時間を計算(delta時間-実際の経過時間)
		int sleep_time_int = int(SEC2MILLI(sleep_time));											//!<sleep時間をSleep関数に渡すため、秒->ミリ秒への変換&int型へキャスト
		sleep_time_int = sleep_time_int > 0 ? sleep_time_int : 0;									//!<誤差や処理落ちでsleep時間が負数になることがあるので、補正
		Sleep(sleep_time_int);																		//!<sleep時間だけアプリケーションを停止
		while (now_time - system_time < delta_time_max) { now_time = Time::GetOSTimeD(); }			//!<もしsleepを行っても経過時間が足りなければ、その時だけwhileで処理を止める
		return 0;
	}

	//時飛ばし(時間の上書きなのであまり多用しないでください)
	void ResetTime() {
		//一応実際のシステム時間は保管する(アニメーションとかは実際の時間をもとに動く)
		sys_time = (unsigned long long)(SEC2MICRO(GetOSTimeD()));			//!<システム上の時間を上書き
		sys_time_prev = sys_time;											//!<前フレームの時間を上書き
	}

	//描画処理の経過時間を初期化・描画FPSを計測
	void FixDrawFPS() {
		draw_fps = 1.0 / draw_delta_time;				//!<描画FPSを計測
		draw_delta_time = 0;							//!<そして経過時間をリセット(描画経過時間は、デクリメントすると莫大な数値になるので0にする)
	}

	// 物理処理の経過時間を更新・物理処理のFPSを計測
	void FixFixedFPS() {
		fixed_fps = 1.0 / fixed_delta_time;						//!<物理FPSを計測
		fixed_delta_time -= fixed_delta_time_max;				//!<そして1フレーム分デクリメント(こちらは0にすると物理演算が不正確になるのでデクリメントして使う)
	}


	//タイムスケールの取得(float)
	const float TimeScale()
	{
		return (float)time_scale;
	}

	//タイムスケールの取得(double)
	const double TimeScaleD()
	{
		return time_scale;
	}

	//タイムスケールの変更
	void SetTimeScale(const double scale)
	{
		time_scale = scale;
	}

	//ゲーム内Δ時間の取得(float)
	const float DeltaTime()
	{
		return (float)(delta_time * time_scale);
	}

	//ゲーム内Δ時間の取得(double)
	const double DeltaTimeD()
	{
		return delta_time * time_scale;
	}

	//描画用Δ時間の取得(float)
	const float DrawDeltaTime()
	{
		return  (float)draw_delta_time;
	}

	//描画用Δ時間の取得(double)
	const double DrawDeltaTimeD()
	{
		return draw_delta_time;
	}

	//物理用Δ時間の取得(float)
	const float FixedDeltaTime()
	{
		return  (float)fixed_delta_time;
	}

	//物理用Δ時間の取得(double)
	const double FixedDeltaTimeD()
	{
		return fixed_delta_time;
	}

	//前フレームとの物理的時間差の取得(double)
	const double UnscaledDeltaTimeD()
	{
		return delta_time;
	}

	//前フレームとの実際の物理的時間差の取得(float)
	const float RealDeltaTime()
	{
		return (float)real_delta_time;
	}

	//前フレームとの実際の物理的時間差の取得(double)
	const float RealDeltaTimeD()
	{
		return real_delta_time;
	}

	//前フレームとの物理的時間差の取得(float)
	const float UnscaledDeltaTime()
	{
		return (float)delta_time;
	}

	//アプリケーション開始後の物理的時間の取得(float)
	const float SystemTimeFromStart()
	{
		return (float)SystemTimeFromStartD();
	}

	//アプリケーション開始後の物理的時間の取得(double)
	const double SystemTimeFromStartD()
	{
		return MICRO2SEC((double)(sys_time - sys_time_start));
	}

	//アプリケーション開始後のゲーム内時間の取得(float)
	const float GetTimeFromStart()
	{
		return (float)time;
	}

	//アプリケーション開始後のゲーム内時間の取得(double)
	const double GetTimeFromStartD()
	{
		return time;
	}

	//アプリケーション開始後の現実時間の取得(float)
	const double GetRealTimeFromStart()
	{
		return (float)real_time;
	}
	//アプリケーション開始後の現実時間の取得(double)
	const double GetRealTimeFromStartD()
	{
		return real_time;
	}
	//システム内時間の取得(float)
	const float SystemTime()
	{
		return (float)SystemTimeD();
	}

	//システム内時間の取得(double)
	const double SystemTimeD()
	{
		return MICRO2SEC((double)sys_time);
	}

	//Windowsから取得した現在時刻(float)
	const float GetOSTime()
	{

		return (float)GetOSTimeD();
	}

	//Windowsから取得した現在時刻(double)
	const double GetOSTimeD()
	{

		//あんまりよく分からんけど、OSの時間を取得
		LARGE_INTEGER integer;
		LARGE_INTEGER integer2;
		QueryPerformanceCounter(&integer);
		QueryPerformanceFrequency(&integer2);
		return (integer.QuadPart / (double)integer2.QuadPart);
	}

	//FPSの最大値を取得(float)
	const float GetFPSMAX()
	{
		return (float)fps_max;
	}

	//FPSの最大値を取得(double)
	const double GetFPSMAXD()
	{
		return fps_max;
	}

	//描画FPSの最大値を取得(float)
	const float GetDrawFPSMAX()
	{
		return (float)draw_fps_max;
	}

	//描画FPSの最大値を取得(double)
	const double GetDrawFPSMAXD()
	{
		return draw_fps_max;
	}

	//FPSの最大値を設定
	void SetFPSMAX(const double& max)
	{
		//FPSの最大値が1を下回るとゼロ除算が起こったりいろいろと危険なので、例外スロー
		fps_max = max;
		if (fps_max < 1)
		{
			throw(Exception("FPS_MAX_LOWER_ZERO", DEFAULT_EXCEPTION_PARAM));
		}
		delta_time_max = 1.0 / fps_max;		//!<渡されたFPS_MAXから、delta時間の最大値を再設定
	}

	//物理更新FPSの最大値を設定
	void SetFixedFPSMAX(const double& max)
	{
		fixed_fps_max = max > fps_max ? fps_max : max;

		//FIXED_FPSの最大値が1を下回るとゼロ除算が起こったりいろいろと危険なので、例外スロー
		if (fixed_fps_max < 1)
		{
			throw(Exception("FIXED_FPS_MAX_LOWER_ZERO", DEFAULT_EXCEPTION_PARAM));
		}
		fixed_delta_time_max = 1.0 / fixed_fps_max;		//!<渡されたFIXED_FPS_MAXから、delta時間の最大値を再設定
	}


	//描画FPSの最大値を設定
	void SetDrawFPSMAX(const double& max)
	{
		draw_fps_max = max;
		//DRAW_FPSの最大値が1を下回るとゼロ除算が起こったりいろいろと危険なので、例外スロー
		if (draw_fps_max < 1)
		{
			throw(Exception("DRAW_FPS_MAX_LOWER_ZERO", DEFAULT_EXCEPTION_PARAM));
		}
		draw_delta_time_max = 1.0 / draw_fps_max;		//!<渡されたDRAW_FPS_MAXから、delta時間の最大値を再設定
	}

	//物理Δ時間の最大値取得(float)
	const float GetFixedDeltaTimeMAX()
	{
		return (float)fixed_delta_time_max;
	}

	//物理Δ時間の最大値取得(double)
	const double GetFixedDeltaTimeMAXD()
	{
		return fixed_delta_time_max;
	}

	//描画Δ時間の最大値取得(float)
	const float GetDrawDeltaTimeMAX()
	{
		return (float)draw_delta_time_max;
	}

	//描画Δ時間の最大値取得(double)
	const double GetDrawDeltaTimeMAXD()
	{
		return draw_delta_time_max;
	}

	//Δ時間の最大値取得(float)
	const float GetDeltaTimeMAX()
	{
		return (float)delta_time_max;
	}

	//Δ時間の最大値取得(double)
	const double GetDeltaTimeMAXD()
	{
		return delta_time_max;
	}

	//FPSの取得(float)
	const float GetFPS()
	{
		return (float)fps;
	}

	//FPSの取得(double)
	const double GetFPSD()
	{
		return fps;
	}
}