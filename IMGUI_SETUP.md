# Подключение Dear ImGui

## 1. Скачать ImGui

https://github.com/ocornut/imgui/releases  (или `git clone`)

Нужна версия **1.90+**.

## 2. Скопировать файлы

Создать папку `DefaultAudioChanger/imgui/` и скопировать туда:

```
imgui.h
imgui.cpp
imgui_draw.cpp
imgui_tables.cpp
imgui_widgets.cpp
imgui_internal.h
imconfig.h
imstb_rectpack.h
imstb_textedit.h
imstb_truetype.h
backends/imgui_impl_win32.h
backends/imgui_impl_win32.cpp
backends/imgui_impl_dx11.h
backends/imgui_impl_dx11.cpp
```

## 3. Собрать

Открыть `DefaultAudioChanger.sln` в Visual Studio 2015+ и собрать в конфигурации Release|x64.

## Что изменилось

- `MainDlg.h/cpp` и `AboutDlg.h/cpp` — больше не используются (можно удалить)
- `ImGuiWindow.h/cpp` — новое окно на ImGui + Direct3D 11
- `DefaultAudioChanger.cpp` — упрощён, WTL диалог заменён на `ImGuiWindow`
- Вся логика работы с устройствами (`DevicesManager`) осталась без изменений
