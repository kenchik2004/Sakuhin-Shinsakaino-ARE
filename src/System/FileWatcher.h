//---------------------------------------------------------------------------
//! @file   FileWatcher.h
//! @brief  ファイル変更監視
//---------------------------------------------------------------------------
#pragma once

//===========================================================================
//! ファイル変更監視
//===========================================================================
class FileWatcher
{
public:
    // 初期化
    //! @param  [in]    path    監視対象のディレクトリパス
    //! @param  [in]    callback    ファイル変更通知のコールバック関数
    bool initialize(const wchar_t* path, std::function<void(const wchar_t*)> callback);

    // 更新
    //! @note メインループで呼ぶ必要がある
    void update();

    // コンストラクタ
    FileWatcher() = default;

    // デストラクタ
    virtual ~FileWatcher();

private:
    // ファイル監視を開始
    bool start();

private:
    HANDLE                              finish_event_     = INVALID_HANDLE_VALUE;    //!< 非同期IO完了通知イベント
    HANDLE                              handle_directory_ = INVALID_HANDLE_VALUE;    //!< 監視先ディレクトリハンドル
    std::vector<u8>                     buffer_;                                     //!< ファイルリスト受信用バッファ
    OVERLAPPED                          overlapped_ = {};                            //!< 非同期IO
    std::function<void(const wchar_t*)> callback_;                                   //!< ファイル変更時のコールバック関数
    std::wstring                        full_path_;                                  //!< 監視中のフルパス
};
