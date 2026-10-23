// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ChatTypes.h"

FChatMessage::FChatMessage()
	:	SenderNickname{TEXT("")},
		MessageText{TEXT("")},
		Channel{EChatChannel::Team},
		SenderTeamId{ 0 },
		Hour{ 0 },
		Minute{ 0 },
		Second{ 0 }
{
}
