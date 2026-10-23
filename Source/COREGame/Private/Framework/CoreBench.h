// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// 측정용 스위치: 서버 실행 인자로 최적화를 꺼서 기준선(Baseline)을 잰다.
// 예) -NoOptAll   또는   -NoOptMinionAnim -NoOptPopupFilter
namespace CoreBench
{
	inline bool IsOptOff(const TCHAR* SwitchName)
	{
		const TCHAR* CmdLine = FCommandLine::Get();
		return FParse::Param(CmdLine, TEXT("NoOptAll")) || FParse::Param(CmdLine, SwitchName);
	}
}