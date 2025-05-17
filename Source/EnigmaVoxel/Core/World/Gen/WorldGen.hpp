#pragma once

class UEnigmaWorld;
struct FChunkHolder;

struct FWorldGen
{
	static void GenerateFullChunk(UEnigmaWorld* World, FChunkHolder& H);
	static void RebuildMesh(UEnigmaWorld* World, FChunkHolder& H);
};
