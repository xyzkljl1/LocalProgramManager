# LocalProgramManager

一个用于保持多个本地程序持续运行的简单管理器。

> 本文档由 AI（OpenAI GPT-6 Codex）生成。

## 面向 Codex 和其他工具的本地控制接口

`LocalProgramManager.exe` 支持两种运行模式。不带参数启动时，程序会像以前一样运行托盘应用。使用 `--control` 启动同一个可执行文件时，它会通过 `QLocalSocket` 向正在运行的托盘应用发送一次本地请求，等待执行结果后退出，无需提供单独的控制程序。

管理器必须已经处于运行状态。程序名称、源目录和工作目录均来自管理器配置，调用方不能提供任意源路径或目标路径。控制接口只提供部署和重启功能。

```powershell
# 替换为实际运行目录中的可执行文件完整路径。
$manager = '<LocalProgramManager.exe 的完整路径>'

# 仅重启程序，不复制文件。
$process = Start-Process -FilePath $manager `
    -ArgumentList @('--control', 'restart', 'PictureSpider') `
    -NoNewWindow -Wait -PassThru
if ($process.ExitCode -ne 0) { throw "重启失败，退出码：$($process.ExitCode)" }

# 项目构建成功后，将已配置的源目录复制到工作目录，
# 并替换当前正在运行的程序。
$process = Start-Process -FilePath $manager `
    -ArgumentList @('--control', 'deploy', 'PictureSpider') `
    -NoNewWindow -Wait -PassThru
if ($process.ExitCode -ne 0) { throw "部署失败，退出码：$($process.ExitCode)" }
```

每条命令都会向标准输出写入一个紧凑的 JSON 对象。自动化工具必须将进程退出码作为判断执行结果的权威依据。如果程序原来处于启用状态，只有替换后的进程持续运行三秒，部署才会以退出码 `0` 表示成功。文件复制或程序启动失败时会返回错误，不会备份或还原旧文件。

该可执行文件仍然是 Windows GUI 应用，因此正常启动时不会打开控制台。PowerShell 使用 `&` 调用 GUI 可执行文件时不会等待其退出；自动化工具必须像上面的示例一样使用 `Start-Process -Wait -PassThru`。`-NoNewWindow` 可让 JSON 响应写入调用方的标准输出。

| 退出码 | 含义 |
| ---: | --- |
| 0 | 成功 |
| 1 | 参数或命令无效 |
| 2 | 正在运行的管理器或控制端点不可用 |
| 3 | 程序名称未知 |
| 4 | 部署或重启失败 |
| 5 | 正在运行的管理器未能在十分钟内完成请求 |
| 6 | 该程序已有另一个部署任务正在执行 |

部署操作会校验源目录和程序文件，然后停止进程，将源目录直接覆盖复制到工作目录，并重新启动程序。配置的源目录中不存在的文件会保留不变，与现有的 Fetch 行为一致。部署过程不会创建暂存目录或备份目录，也不会在失败时回滚。
