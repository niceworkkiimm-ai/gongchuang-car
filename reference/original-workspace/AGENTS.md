# 工创项目入口与协作要求

- 用户于2026-10-09确认：此文件夹保留旧版工程、初版备份和赛题。当前代码修改及Keil烧录使用 `C:\Users\Kim\Desktop\小车只加通信版本`，不要用本文件夹旧工程覆盖现版本。
- 本地Git仓库：`D:\工创Git仓库\gongchuang-car`，当前主控备份在 `stm32/`。
- 后续修改当前C盘工程，保留原有编码、字节和换行；验证后同步D盘仓库，并将同一提交上传个人仓库 `https://github.com/niceworkkiimm-ai/gongchuang-car` 和协作仓库 `https://github.com/niceworkkiimm-ai/gongchuang-car-team`。
- 操作前阅读D盘仓库的 `AGENTS.md` 与 `docs/项目接手笔记-2026-10-09.md`。执行 `tools/backup.ps1` 同时推送origin和team；有协作者新提交时先审查整合，不强推覆盖。
- 只询问原因或要求检查时，先分析，不擅自改控制代码。局部顺序修改不能通过互换全局舵机角度实现；新增测试不自动切换main入口。
- 本文件夹原有 `工创项目学习笔记.md` 描述早期版本，不能作为当前参数依据。当前C盘源码和用户新要求优先。
