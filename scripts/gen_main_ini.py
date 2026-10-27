"""Generates rainmeter/ToDo/Main/Main.ini (ASCII, CRLF): static sections plus the repeated rows.

Main.ini is a generated file: edit this script and run it. `--check` compares instead of writing and exits
with 1 when Main.ini differs. Rows are numbered from 1; "@I@" in a row template is the row number. The row
counts are written into the script measures' options (RowCount, BlockRows, ChangeRows, UnplannedRows,
ListRows), which task_view.lua, plan_view.lua, and chat_view.lua read.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

TARGET: Path = Path(__file__).resolve().parent.parent / "rainmeter" / "ToDo" / "Main" / "Main.ini"
ROW_NUMBER: str = "@I@"

# The [Rainmeter] section, includes, variables, meter styles, measures, the header, the tabs, and the task scope bar.
HEAD: str = r'''[Rainmeter]
Update=1000
AccurateText=1
DynamicWindowSize=1
OnRefreshAction=[!CommandMeasure MeasureChat "Start()"][!CommandMeasure MeasureTasks "Start()"]

[Metadata]
Name=ToDo
Information=Tasks, plans, and DeepSeek conversations backed by todo.exe. All content changes go through todo.exe.

; ui_state.inc defines Style (a | b | c) before the style file is chosen.
@Include1=#@#paths.inc
@Include2=#@#ui_state.inc
@Include3=#@#style-#Style#.inc

[Variables]
SegX=(#SkinWidth#-#PadX#-96)
HeaderMid=(#PadY#+#HeaderH#/2)
BarY=(#PadY#+#HeaderH#+4)

; ---------------------------------------------------------------- meter styles

[StyleText]
FontFace=#FontFace#
FontSize=#Body#
FontColor=#ColorText#
AntiAlias=1
SolidColor=0,0,0,1
ClipString=1

[StyleDim]
FontFace=#FontFace#
FontSize=#Meta#
FontColor=#ColorDim#
AntiAlias=1
SolidColor=0,0,0,1
ClipString=1

[StyleChip]
FontFace=#FontFace#
FontSize=#Meta#
FontColor=#ColorDim#
AntiAlias=1
SolidColor=#ColorPanel#
Padding=6,1,6,2
StringAlign=LeftCenter

[StyleSeg]
FontFace=#FontFace#
FontSize=#Meta#
FontColor=#ColorDim#
AntiAlias=1
SolidColor=0,0,0,1
StringAlign=CenterCenter
Y=#HeaderMid#
H=18

[StyleRowText]
FontFace=#FontFace#
FontSize=#Body#
FontColor=#ColorText#
AntiAlias=1
SolidColor=0,0,0,1
ClipString=1
StringAlign=LeftCenter
Hidden=1

[StyleRowMeta]
FontFace=#FontFace#
FontSize=#Meta#
FontColor=#ColorDim#
AntiAlias=1
SolidColor=0,0,0,1
ClipString=1
StringAlign=RightCenter
X=(#SkinWidth#-#PadX#-4)
Hidden=1

[StyleRowShape]
Shape=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape2=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape3=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape4=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Hidden=1

; ---------------------------------------------------------------- measures

[MeasureTasks]
Measure=Script
ScriptFile=#@#Scripts\task_view.lua
RowCount=@TASK_ROW_COUNT@
UiStateFile=#@#ui_state.inc

[MeasurePlan]
Measure=Script
ScriptFile=#@#Scripts\plan_view.lua
BlockRows=@TIMELINE_ROW_COUNT@
ChangeRows=@CHANGE_ROW_COUNT@
UnplannedRows=@UNPLANNED_ROW_COUNT@

[MeasureChat]
Measure=Script
ScriptFile=#@#Scripts\chat_view.lua
ListRows=@LIST_ROW_COUNT@
UiStateFile=#@#ui_state.inc
MaxChatHeight=240

[MeasureTaskList]
Measure=Plugin
Plugin=RunCommand
Program="#TodoExe#"
Parameter=
OutputType=UTF8
State=Hide
Timeout=30000
DynamicVariables=1
FinishAction=[!CommandMeasure MeasureTasks "OnListFinished()"]

[MeasureTaskAction]
Measure=Plugin
Plugin=RunCommand
Program="#TodoExe#"
Parameter=
OutputType=UTF8
State=Hide
Timeout=30000
DynamicVariables=1
FinishAction=[!CommandMeasure MeasureTasks "OnActionFinished()"]

[MeasureTaskPrompt]
Measure=Plugin
Plugin=RunCommand
Program="#TodoExe#"
Parameter=
OutputType=UTF8
State=Hide
Timeout=-1
DynamicVariables=1
FinishAction=[!CommandMeasure MeasureTasks "OnPromptFinished()"]

[MeasurePlanRun]
Measure=Plugin
Plugin=RunCommand
Program="#TodoExe#"
Parameter=
OutputType=UTF8
State=Hide
Timeout=240000
DynamicVariables=1
FinishAction=[!CommandMeasure MeasurePlan "OnRunFinished()"]

[MeasurePlanAction]
Measure=Plugin
Plugin=RunCommand
Program="#TodoExe#"
Parameter=
OutputType=UTF8
State=Hide
Timeout=30000
DynamicVariables=1
FinishAction=[!CommandMeasure MeasurePlan "OnActionFinished()"]

[MeasureChatRun]
Measure=Plugin
Plugin=RunCommand
Program="#TodoExe#"
Parameter=
OutputType=UTF8
State=Hide
Timeout=240000
DynamicVariables=1
FinishAction=[!CommandMeasure MeasureChat "OnChatFinished()"]

[MeasureChatList]
Measure=Plugin
Plugin=RunCommand
Program="#TodoExe#"
Parameter=
OutputType=UTF8
State=Hide
Timeout=30000
DynamicVariables=1
FinishAction=[!CommandMeasure MeasureChat "OnListFinished()"]

[MeasureSkillList]
Measure=Plugin
Plugin=RunCommand
Program="#TodoExe#"
Parameter=
OutputType=UTF8
State=Hide
Timeout=30000
DynamicVariables=1
FinishAction=[!CommandMeasure MeasureChat "OnSkillsFinished()"]

[MeasureChatPrompt]
Measure=Plugin
Plugin=RunCommand
Program="#TodoExe#"
Parameter=
OutputType=UTF8
State=Hide
Timeout=-1
DynamicVariables=1
FinishAction=[!CommandMeasure MeasureChat "OnPromptFinished()"]

[MeasureTick]
Measure=Calc
Formula=(MeasureTick + 1) % 60
IfCondition=MeasureTick = 0
IfTrueAction=[!CommandMeasure MeasureTasks "Tick()"]

; ---------------------------------------------------------------- background (chat_view.lua sets its height)

[MeterBackground]
Meter=Shape
Shape=Rectangle 0.5,0.5,(#SkinWidth#-1),120,#Radius# | Fill Color #ColorBg# | Stroke Color #ColorStroke# | StrokeWidth 1

; ---------------------------------------------------------------- header (task_view.lua)

[MeterDate]
Meter=String
MeterStyle=StyleText
X=#PadX#
Y=#HeaderMid#
FontSize=#TitleSize#
FontWeight=#TitleWeight#
StringAlign=LeftCenter
ClipString=0
Text=
ToolTipText=Click: sync hand edits and reload
LeftMouseUpAction=[!CommandMeasure MeasureTasks "OnSyncClick()"]

[MeterSub]
Meter=String
MeterStyle=StyleDim
X=8R
Y=#HeaderMid#
StringAlign=LeftCenter
ClipString=0
Text=

[ButtonStyle]
Meter=String
MeterStyle=StyleChip
X=8R
Y=#HeaderMid#
FontSize=#Small#
Text=#StyleName#
ToolTipText=Style #StyleName#. Click: next style (A / B / C)
LeftMouseUpAction=[!CommandMeasure MeasureTasks "CycleStyle()"]


[SegBackground]
Meter=Shape
X=#SegX#
Y=(#HeaderMid#-10)
Shape=Rectangle 0,0,96,20,5 | Fill Color #ColorPanel# | StrokeWidth 0

[SegSelected]
Meter=Shape
X=(#SegX#+2)
Y=(#HeaderMid#-8)
Shape=Rectangle 0,0,44,16,4 | Fill Color #ColorSegOn# | StrokeWidth 0

[SegPlan]
Meter=String
MeterStyle=StyleSeg
X=(#SegX#+24)
W=44
Text=Plan
LeftMouseUpAction=[!CommandMeasure MeasureTasks "SetView('plan')"]

[SegTasks]
Meter=String
MeterStyle=StyleSeg
X=(#SegX#+71)
W=46
Text=Tasks
LeftMouseUpAction=[!CommandMeasure MeasureTasks "SetView('tasks')"]

[TaskBars]
Meter=Shape
X=#PadX#
Y=#BarY#
Shape=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape2=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape3=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape4=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape5=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape6=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape7=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape8=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape9=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape10=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape11=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape12=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape13=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape14=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape15=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape16=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape17=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape18=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape19=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape20=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape21=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape22=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape23=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape24=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0

; ---------------------------------------------------------------- Tasks tab toolbar (task_view.lua sets Y)

[TaskScopeDay]
Meter=String
MeterStyle=StyleChip
X=#PadX#
Hidden=1
Text=day
LeftMouseUpAction=[!CommandMeasure MeasureTasks "SetScope('day')"]

[TaskScopeWeek]
Meter=String
MeterStyle=StyleChip
X=4R
Hidden=1
Text=week
LeftMouseUpAction=[!CommandMeasure MeasureTasks "SetScope('week')"]

[TaskScopeLong]
Meter=String
MeterStyle=StyleChip
X=4R
Hidden=1
Text=long
LeftMouseUpAction=[!CommandMeasure MeasureTasks "SetScope('long')"]

[ButtonAdd]
Meter=String
MeterStyle=StyleChip
X=(#SkinWidth#-#PadX#)
StringAlign=RightCenter
FontColor=#ColorAccent#
Hidden=1
Text=+ add
ToolTipText=Add a task to this list
LeftMouseUpAction=[!CommandMeasure MeasureTasks "OnAddClick()"]

; ---------------------------------------------------------------- task rows (task_view.lua sets Y)

[RowHover]
Meter=Shape
X=(#PadX#-4)
Shape=Rectangle 0,0,(#SkinWidth#-2*#PadX#+8),#RowH#,4 | Fill Color #ColorHover# | StrokeWidth 0
Hidden=1

'''

# One task row of the Tasks tab: status, place dot and letter, title, meta (task_view.lua).
TASK_ROW: str = r'''[Status@I@]
Meter=Shape
MeterStyle=StyleRowShape
X=#PadX#
ToolTipText=Click: todo > doing > done. Right-click: blocked / dropped
LeftMouseUpAction=[!CommandMeasure MeasureTasks "OnStatusClick(@I@)"]
RightMouseUpAction=[!CommandMeasure MeasureTasks "OnStatusAltClick(@I@)"]
[PlaceDot@I@]
Meter=Shape
MeterStyle=StyleRowShape
X=(#PadX#+22)
[PlaceText@I@]
Meter=String
MeterStyle=StyleRowText
X=(#PadX#+30)
W=16
H=16
FontSize=6.5
FontWeight=700
FontColor=#ColorOnPlace#
StringAlign=CenterCenter
ToolTipText=Click: place any > lab > class > remote
LeftMouseUpAction=[!CommandMeasure MeasureTasks "OnPlaceClick(@I@)"]
[Title@I@]
Meter=String
MeterStyle=StyleRowText
X=(#PadX#+44)
W=272
H=#RowH#
MouseOverAction=[!CommandMeasure MeasureTasks "Hover(@I@)"]
MouseLeaveAction=[!CommandMeasure MeasureTasks "Hover(0)"]
LeftMouseUpAction=[!CommandMeasure MeasureTasks "OnNoteClick(@I@)"]
[Meta@I@]
Meter=String
MeterStyle=StyleRowMeta
W=72
InlinePattern=r[0-9]+$
InlineSetting=Color | #ColorRemote#

'''
TASK_ROW_COUNT: int = 10

# After the task rows: the task pager, then the Plan tab header and its empty state.
TASK_FOOT_AND_PLAN_HEAD: str = r'''[TaskEmpty]
Meter=String
MeterStyle=StyleDim
X=(#PadX#+4)
StringAlign=LeftCenter
Hidden=1
Text=No tasks. Click + to add one.

[TaskPrev]
Meter=String
MeterStyle=StyleRowMeta
X=(#SkinWidth#-#PadX#-56)
Text=<
LeftMouseUpAction=[!CommandMeasure MeasureTasks "ChangePage(-1)"]
[TaskPage]
Meter=String
MeterStyle=StyleRowMeta
X=(#SkinWidth#-#PadX#-18)
Text=1/1
[TaskNext]
Meter=String
MeterStyle=StyleRowMeta
X=(#SkinWidth#-#PadX#-4)
Text=>
LeftMouseUpAction=[!CommandMeasure MeasureTasks "ChangePage(1)"]

; ---------------------------------------------------------------- Plan tab (plan_view.lua sets Y; Group=Plan)

[PlanBars]
Meter=Shape
X=#PadX#
Y=#BarY#
Group=Plan
Hidden=1
Shape=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape2=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape3=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape4=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape5=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape6=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape7=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape8=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape9=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape10=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape11=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape12=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape13=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape14=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape15=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape16=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape17=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape18=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape19=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape20=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape21=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape22=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape23=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0
Shape24=Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0

[PlanScopeDay]
Meter=String
MeterStyle=StyleChip
X=#PadX#
Group=Plan
Hidden=1
Text=day
LeftMouseUpAction=[!CommandMeasure MeasurePlan "SetScope('day')"]

[PlanScopeWeek]
Meter=String
MeterStyle=StyleChip
X=4R
Group=Plan
Hidden=1
Text=week
LeftMouseUpAction=[!CommandMeasure MeasurePlan "SetScope('week')"]

[PlanState]
Meter=String
MeterStyle=StyleDim
X=10R
StringAlign=LeftCenter
ClipString=0
Group=Plan
Hidden=1
Text=

[PlanGenerate]
Meter=String
MeterStyle=StyleChip
X=(#SkinWidth#-#PadX#)
StringAlign=RightCenter
FontColor=#ColorAccent#
Group=Plan
Hidden=1
Text=plan
ToolTipText=Ask DeepSeek for a new plan (replaces the stored one)
LeftMouseUpAction=[!CommandMeasure MeasurePlan "Generate()"]

[PlanEmpty]
Meter=String
MeterStyle=StyleDim
X=(#PadX#+4)
StringAlign=LeftCenter
Group=Plan
Hidden=1
Text=

'''

# One block of the Plan timeline: time, node, title, duration (plan_view.lua).
TIMELINE_ROW: str = r'''[TlTime@I@]
Meter=String
MeterStyle=StyleRowMeta
X=(#PadX#+66)
W=66
Group=Plan
[TlNode@I@]
Meter=Shape
MeterStyle=StyleRowShape
X=(#PadX#+72)
Group=Plan
LeftMouseUpAction=[!CommandMeasure MeasurePlan "OnBlockStatusClick(@I@)"]
[TlTitle@I@]
Meter=String
MeterStyle=StyleRowText
X=(#PadX#+90)
W=262
Group=Plan
[TlDur@I@]
Meter=String
MeterStyle=StyleRowMeta
W=44
Group=Plan

'''
TIMELINE_ROW_COUNT: int = 12

# After the timeline rows: the timeline pager, the changes label, and the rationale.
TIMELINE_FOOT: str = r'''[TlPrev]
Meter=String
MeterStyle=StyleRowMeta
X=(#SkinWidth#-#PadX#-56)
Group=Plan
Text=<
LeftMouseUpAction=[!CommandMeasure MeasurePlan "ChangePage(-1)"]
[TlPage]
Meter=String
MeterStyle=StyleRowMeta
X=(#SkinWidth#-#PadX#-18)
Group=Plan
Text=1/1
[TlNext]
Meter=String
MeterStyle=StyleRowMeta
X=(#SkinWidth#-#PadX#-4)
Group=Plan
Text=>
LeftMouseUpAction=[!CommandMeasure MeasurePlan "ChangePage(1)"]

[ChangesLabel]
Meter=String
MeterStyle=StyleDim
X=#PadX#
StringAlign=LeftCenter
ClipString=0
Group=Plan
Hidden=1
Text=CHANGES

[ButtonWhy]
Meter=String
MeterStyle=StyleChip
X=8R
FontSize=#Small#
Group=Plan
Hidden=1
Text=why
ToolTipText=Show or hide the rationale
LeftMouseUpAction=[!CommandMeasure MeasurePlan "ToggleWhy()"]

[RationaleText]
Meter=String
MeterStyle=StyleDim
X=#PadX#
W=(#SkinWidth#-2*#PadX#)
ClipString=2
Group=Plan
Hidden=1
Text=

'''

# One proposed change with its checkbox (plan_view.lua).
CHANGE_ROW: str = r'''[ChangeBox@I@]
Meter=Shape
MeterStyle=StyleRowShape
X=(#PadX#+2)
Group=Plan
LeftMouseUpAction=[!CommandMeasure MeasurePlan "ToggleChange(@I@)"]
[ChangeText@I@]
Meter=String
MeterStyle=StyleRowText
X=(#PadX#+22)
W=(#SkinWidth#-2*#PadX#-22)
H=#ChangeRowH#
Group=Plan
LeftMouseUpAction=[!CommandMeasure MeasurePlan "ToggleChange(@I@)"]

'''
CHANGE_ROW_COUNT: int = 8

# After the change rows: the unplanned label.
CHANGES_FOOT: str = r'''[UnplannedLabel]
Meter=String
MeterStyle=StyleDim
X=#PadX#
StringAlign=LeftCenter
ClipString=0
Group=Plan
Hidden=1
Text=NOT IN PLAN

'''

# One task the plan does not schedule (plan_view.lua).
UNPLANNED_ROW: str = r'''[UnplannedMark@I@]
Meter=Shape
MeterStyle=StyleRowShape
X=#PadX#
Group=Plan
ToolTipText=Click: todo > doing > done
LeftMouseUpAction=[!CommandMeasure MeasurePlan "OnUnplannedClick(@I@)"]
[UnplannedText@I@]
Meter=String
MeterStyle=StyleRowText
X=(#PadX#+22)
W=(#SkinWidth#-2*#PadX#-80)
Group=Plan
[UnplannedMeta@I@]
Meter=String
MeterStyle=StyleRowMeta
W=56
Group=Plan

'''
UNPLANNED_ROW_COUNT: int = 6

# After the unplanned rows: the plan buttons, the ask bar, the conversation, and the list header.
PLAN_FOOT_AND_CHAT: str = r'''[ButtonDiscard]
Meter=String
MeterStyle=StyleChip
X=(#SkinWidth#-#PadX#-70)
StringAlign=RightCenter
SolidColor=0,0,0,1
Group=Plan
Hidden=1
Text=discard
LeftMouseUpAction=[!CommandMeasure MeasurePlan "Discard()"]

[ButtonApply]
Meter=String
MeterStyle=StyleChip
X=(#SkinWidth#-#PadX#)
StringAlign=RightCenter
SolidColor=#ColorSegOn#
FontColor=#ColorAccent#
Group=Plan
Hidden=1
Text=apply
LeftMouseUpAction=[!CommandMeasure MeasurePlan "Apply()"]

; ---------------------------------------------------------------- ask bar and conversation (chat_view.lua sets Y)

[AskLine]
Meter=Shape
X=#PadX#
Shape=Rectangle 0,0,(#SkinWidth#-2*#PadX#),1 | Fill Color #ColorLine# | StrokeWidth 0

[AskPrompt]
Meter=String
MeterStyle=StyleText
X=(#PadX#+2)
FontFace=#FontMono#
FontColor=#ColorAccent#
StringAlign=LeftCenter
Text=>
LeftMouseUpAction=[!CommandMeasure MeasureChat "OnAskClick()"]

[AskPlaceholder]
Meter=String
MeterStyle=StyleText
X=(#PadX#+16)
W=200
FontColor=#ColorDim#
StringAlign=LeftCenter
Text=Ask...
LeftMouseUpAction=[!CommandMeasure MeasureChat "OnAskClick()"]

[ButtonSkill]
Meter=String
MeterStyle=StyleChip
StringAlign=RightCenter
Text=no skill
ToolTipText=Skill for the next conversation. Click: next skill
LeftMouseUpAction=[!CommandMeasure MeasureChat "CycleSkill()"]

[ButtonTasks]
Meter=String
MeterStyle=StyleChip
StringAlign=RightCenter
Text=tasks
ToolTipText=Include the task lists in the question
LeftMouseUpAction=[!CommandMeasure MeasureChat "ToggleTasks()"]

[ButtonChats]
Meter=Shape
X=(#SkinWidth#-#PadX#-22)
Shape=Rectangle 0,0,22,17,4 | Fill Color #ColorPanel# | StrokeWidth 0
Shape2=Line 7,5.5,15,5.5 | StrokeWidth 1.2 | Stroke Color #ColorDim#
Shape3=Line 7,8.5,15,8.5 | StrokeWidth 1.2 | Stroke Color #ColorDim#
Shape4=Line 7,11.5,15,11.5 | StrokeWidth 1.2 | Stroke Color #ColorDim#
ToolTipText=Conversations
LeftMouseUpAction=[!CommandMeasure MeasureChat "ToggleList()"]

[ChatViewport]
Meter=Shape
X=#PadX#
Shape=Rectangle 0,0,(#SkinWidth#-2*#PadX#),10 | Fill Color 255,255,255,255 | StrokeWidth 0
Hidden=1
MouseScrollUpAction=[!CommandMeasure MeasureChat "Scroll(-1)"]
MouseScrollDownAction=[!CommandMeasure MeasureChat "Scroll(1)"]

[ChatText]
Meter=String
Container=ChatViewport
MeterStyle=StyleText
X=0
Y=0
W=(#SkinWidth#-2*#PadX#)
ClipString=2
Hidden=1
InlinePattern=(?m)^(?:You|AI)(?=  )
InlineSetting=Color | #ColorAccent#
InlinePattern2=(?m)^(?:You|AI)(?=  )
InlineSetting2=Weight | 600
InlinePattern3=(?m)^    .*$
InlineSetting3=Face | #FontMono#
InlinePattern4=(?m)^    .*$
InlineSetting4=Size | #Meta#
InlinePattern5=(?m)^    .*$
InlineSetting5=Color | #ColorDim#
InlinePattern6=(?m)^    (ok)
InlineSetting6=Color | #ColorDone#
InlinePattern7=(?m)^    (denied|error)
InlineSetting7=Color | #ColorBlocked#
Text=

[ChatFoot]
Meter=String
MeterStyle=StyleDim
X=#PadX#
W=300
StringAlign=LeftCenter
FontSize=#Small#
Hidden=1
Text=

[ButtonNewChat]
Meter=String
MeterStyle=StyleChip
X=(#SkinWidth#-#PadX#-24)
StringAlign=RightCenter
FontSize=#Small#
Hidden=1
Text=new
ToolTipText=Start a new conversation
LeftMouseUpAction=[!CommandMeasure MeasureChat "NewChat()"]

[ButtonFold]
Meter=String
MeterStyle=StyleChip
X=(#SkinWidth#-#PadX#)
StringAlign=RightCenter
FontSize=#Small#
Hidden=1
Text=^
ToolTipText=Fold or unfold the conversation
LeftMouseUpAction=[!CommandMeasure MeasureChat "ToggleFold()"]

[ListNew]
Meter=String
MeterStyle=StyleText
X=(#PadX#+4)
W=(#SkinWidth#-2*#PadX#-8)
FontColor=#ColorAccent#
StringAlign=LeftCenter
Group=ChatList
Hidden=1
Text=+ New conversation
LeftMouseUpAction=[!CommandMeasure MeasureChat "NewChat()"]

'''

# One row of the conversation list (chat_view.lua).
LIST_ROW: str = r'''[ListRow@I@]
Meter=String
MeterStyle=StyleText
X=(#PadX#+4)
W=(#SkinWidth#-2*#PadX#-8)
StringAlign=LeftCenter
Group=ChatList
Hidden=1
InlinePattern=^.{12}
InlineSetting=Color | #ColorDim#
LeftMouseUpAction=[!CommandMeasure MeasureChat "SelectRow(@I@)"]

'''
LIST_ROW_COUNT: int = 8

# After the conversation list rows: the list pager and the message line.
CHAT_FOOT: str = r'''[ListPrev]
Meter=String
MeterStyle=StyleRowMeta
X=(#SkinWidth#-#PadX#-56)
Group=ChatList
Text=<
LeftMouseUpAction=[!CommandMeasure MeasureChat "ChangeListPage(-1)"]
[ListPage]
Meter=String
MeterStyle=StyleRowMeta
X=(#SkinWidth#-#PadX#-18)
Group=ChatList
Text=1/1
[ListNext]
Meter=String
MeterStyle=StyleRowMeta
X=(#SkinWidth#-#PadX#-4)
Group=ChatList
Text=>
LeftMouseUpAction=[!CommandMeasure MeasureChat "ChangeListPage(1)"]

[MeterMessage]
Meter=String
MeterStyle=StyleDim
X=#PadX#
W=(#SkinWidth#-2*#PadX#)
FontSize=#Small#
ClipString=2
Hidden=1
Text=
'''


def rows(template: str, count: int) -> str:
    """The template once per row, numbered from 1."""
    return "".join(template.replace(ROW_NUMBER, str(number)) for number in range(1, count + 1))


def head() -> str:
    """HEAD with the row counts the view scripts read (RowCount, BlockRows, ChangeRows, UnplannedRows, ListRows)."""
    counts: dict[str, int] = {
        "@TASK_ROW_COUNT@": TASK_ROW_COUNT,
        "@TIMELINE_ROW_COUNT@": TIMELINE_ROW_COUNT,
        "@CHANGE_ROW_COUNT@": CHANGE_ROW_COUNT,
        "@UNPLANNED_ROW_COUNT@": UNPLANNED_ROW_COUNT,
        "@LIST_ROW_COUNT@": LIST_ROW_COUNT,
    }
    text: str = HEAD
    for marker, count in counts.items():
        text = text.replace(marker, str(count))
    return text


def build() -> str:
    """The whole file with LF line ends."""
    return "".join([
        head(),
        rows(TASK_ROW, TASK_ROW_COUNT),
        TASK_FOOT_AND_PLAN_HEAD,
        rows(TIMELINE_ROW, TIMELINE_ROW_COUNT),
        TIMELINE_FOOT,
        rows(CHANGE_ROW, CHANGE_ROW_COUNT),
        CHANGES_FOOT,
        rows(UNPLANNED_ROW, UNPLANNED_ROW_COUNT),
        PLAN_FOOT_AND_CHAT,
        rows(LIST_ROW, LIST_ROW_COUNT),
        CHAT_FOOT,
    ])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="compare with Main.ini instead of writing it")
    args = parser.parse_args()
    content: bytes = build().replace("\n", "\r\n").encode("ascii")
    if args.check:
        same: bool = TARGET.read_bytes() == content
        print(f"{TARGET.name}: {'up to date' if same else 'differs from the generator'}")
        sys.exit(0 if same else 1)
    TARGET.write_bytes(content)
    print(f"wrote {TARGET} ({len(content)} bytes)")


if __name__ == "__main__":
    main()
