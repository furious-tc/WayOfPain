// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once


/**
 * when you modify this, please note that this information can be saved with instances
 * also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list
 **/

// Trace against Actors/Components which provide interactions.
#define NL_TraceChannel_Traversable				ECC_GameTraceChannel1

// Trace used by Hits, will hit physics assets instead of capsules
#define NL_TraceChannel_Hit						ECC_GameTraceChannel2

// Trace used by by Hits, will hit pawn capsules instead of physics assets
#define NL_TraceChannel_Hit_Capsule				ECC_GameTraceChannel3

// Trace used by by Hits, will trace through multiple pawns rather than stopping on the first hit
#define NL_TraceChannel_Hit_Multi				ECC_GameTraceChannel4

// Trace against Actors/Components which provide interactions.
#define NL_TraceChannel_Interaction				ECC_GameTraceChannel5

// ECC_GameTraceChannel6
