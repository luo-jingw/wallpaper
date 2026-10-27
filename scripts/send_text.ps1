# Types text and keys into the foreground window, but only if that window belongs to -ProcessId.
# Text is sent as Unicode key events, so Chinese characters do not depend on the active IME.
param(
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [string]$Text = "",
    [ValidateSet("None", "Enter", "Escape")][string]$Key = "None"
)

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class KeySender {
    [StructLayout(LayoutKind.Sequential)] struct KEYBDINPUT { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
    [StructLayout(LayoutKind.Explicit, Size = 40)] struct INPUT { [FieldOffset(0)] public uint type; [FieldOffset(8)] public KEYBDINPUT ki; }
    [DllImport("user32.dll")] static extern uint SendInput(uint n, INPUT[] inputs, int size);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    const uint KEYEVENTF_KEYUP = 2, KEYEVENTF_UNICODE = 4;
    static uint Send(ushort vk, ushort scan, uint flags) {
        var inputs = new INPUT[2];
        inputs[0].type = 1; inputs[0].ki.wVk = vk; inputs[0].ki.wScan = scan; inputs[0].ki.dwFlags = flags;
        inputs[1].type = 1; inputs[1].ki.wVk = vk; inputs[1].ki.wScan = scan; inputs[1].ki.dwFlags = flags | KEYEVENTF_KEYUP;
        return SendInput(2, inputs, Marshal.SizeOf(typeof(INPUT)));
    }
    public static uint Text(string text) { uint sent = 0; foreach (char c in text) sent += Send(0, c, KEYEVENTF_UNICODE); return sent; }
    public static uint Key(ushort vk) { return Send(vk, 0, 0); }
}
"@

$foreground = [KeySender]::GetForegroundWindow()
$owner = [uint32]0
[void][KeySender]::GetWindowThreadProcessId($foreground, [ref]$owner)
if ($owner -ne $ProcessId) {
    throw "foreground window belongs to pid $owner, not $ProcessId; nothing sent"
}
$sent = [KeySender]::Text($Text)
switch ($Key) {
    "Enter" { $sent += [KeySender]::Key(0x0D) }
    "Escape" { $sent += [KeySender]::Key(0x1B) }
}
"sent $sent key events to pid $ProcessId"
