# WSL環境でのUSB接続方法

1. windows環境でpowershellを起動（管理者権限を推奨）  
2. `> usbipd list` を実行して接続されているUSBデバイスとWSL環境への接続状態を把握  
    ```powershell
    > usbipd list
    BUSID  VID:PID    DEVICE                                                        STATE
    1-11   0b05:19af  AURA LED Controller, USB 入力デバイス                         Not shared
    3-2    10c4:ea60  Silicon Labs CP210x USB to UART Bridge (COM3)                 Not shared
    4-1    0d8c:4147  USB HIFI AUDIO, USB 入力デバイス                              Not shared
    4-3    046d:c539  G502 LIGHTSPEED, USB 入力デバイス, LIGHTSPEED Receiver,  ...  Not shared
    4-4    320f:5088  USB 入力デバイス                                              Not shared
    ```
3. `> usbipd bind --busid <busid>`　を実行してデバイスを共有し、WSLに接続できるようにする。  
    なお、この操作には**管理者権限が必要**である。
    ```powershell
    > usbipd bind --busid 3-2
    > usbipd list
    Connected:
    BUSID  VID:PID    DEVICE                                                        STATE
    1-11   0b05:19af  AURA LED Controller, USB 入力デバイス                         Not shared
    3-2    10c4:ea60  Silicon Labs CP210x USB to UART Bridge (COM3)                 Shared
    4-1    0d8c:4147  USB HIFI AUDIO, USB 入力デバイス                              Not shared
    4-3    046d:c539  G502 LIGHTSPEED, USB 入力デバイス, LIGHTSPEED Receiver,  ...  Not shared
    4-4    320f:5088  USB 入力デバイス                                              Not shared
    ```
4. `> usbipd attach --wsl --busid <busid>`を実行して、WSLにUSBを接続  
    ```powershell
    > usbipd attach --wsl --busid 3-2
    usbipd: info: Using WSL distribution 'Ubuntu-24.04' to attach; the device will be available in all WSL 2 distributions.
    usbipd: info: Detected networking mode 'nat'.
    usbipd: info: Using IP address 172.29.144.1 to reach the host.

    > usbipd list
    Connected:
    BUSID  VID:PID    DEVICE                                                        STATE
    1-11   0b05:19af  AURA LED Controller, USB 入力デバイス                         Not shared
    3-2    10c4:ea60  Silicon Labs CP210x USB to UART Bridge (COM3)                 Attached
    4-1    0d8c:4147  USB HIFI AUDIO, USB 入力デバイス                              Not shared
    4-3    046d:c539  G502 LIGHTSPEED, USB 入力デバイス, LIGHTSPEED Receiver,  ...  Not shared
    4-4    320f:5088  USB 入力デバイス                                              Not shared

    Persisted:
    GUID                                  DEVICE
    ```

5. WSLにてUSB接続状態を確認
    ```bash
    $ lsusb
    plane@DESKTOP-0CSRS3N:~/Source$ lsusb
    Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
    Bus 001 Device 002: ID 10c4:ea60 Silicon Labs CP210x UART Bridge
    Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
    ```

## アンマウント方法
WSL でデバイスを使い終わったら、USB デバイスの接続を物理的に解除するか、PowerShell から次のコマンドを実行することができます。
```powershell
usbipd detach --busid <busid>
```


# セットアップ方法  
参考ページ：https://learn.microsoft.com/ja-jp/windows/wsl/connect-usb  

- USBIPD-WIN プロジェクトをインストールする
    - [usbipd-win プロジェクトの最新リリース](https://github.com/dorssel/usbipd-win/releases)のページに移動します。  
    - .msi を選択します。これにより、インストーラーがダウンロードされます。  
    - ダウンロードした usbipd-win_x.msi インストーラー ファイルを実行します。  
- Silicon Labs CP210x USB to UART Bridge をインストールする  
    - [ドライバーダウンロードページ](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers)からドライバーをダウンロード  
    - ダウンロードされたファイルを展開  
    - 中にあるinfファイルを実行してインストール  