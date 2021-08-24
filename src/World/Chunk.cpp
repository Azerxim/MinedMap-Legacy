/*
  Copyright (c) 2015-2019, Matthias Schiffer <mschiffer@universe-factory.net>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "Chunk.hpp"
#include "../NBT/IntTag.hpp"
#include "../NBT/ListTag.hpp"
#include "../NBT/StringTag.hpp"

#include <cstring>
#include <stdexcept>


namespace MinedMap {
namespace World {

Chunk::Chunk(const ChunkData *data) {
	level = assertValue(data->getRoot().get<NBT::CompoundTag>("Level"));

	std::shared_ptr<const NBT::ListTag> sectionsTag = level->get<NBT::ListTag>("Sections");
	if (!sectionsTag || sectionsTag->empty())
		return;

	auto biomesIntArray = level->get<NBT::IntArrayTag>("Biomes");
	auto biomesByteArray = level->get<NBT::ByteArrayTag>("Biomes");

	if (biomesIntArray && biomesIntArray->getLength() == BSIZE*BSIZE*BMAXY)
		biomeInts = std::move(biomesIntArray);
	else if (biomesIntArray && biomesIntArray->getLength() == SIZE*SIZE)
		biomeIntsPre115 = std::move(biomesIntArray);
	else if (biomesByteArray && biomesByteArray->getLength() == SIZE*SIZE)
		biomeBytes = std::move(biomesByteArray);
	else
		throw std::invalid_argument("corrupt biome data");

	auto dataVersionTag = data->getRoot().get<NBT::IntTag>("DataVersion");
	uint32_t dataVersion = dataVersionTag ? dataVersionTag->getValue() : 0;

	for (auto &sTag : *sectionsTag) {
		auto s = std::dynamic_pointer_cast<const NBT::CompoundTag>(sTag);
		std::unique_ptr<Section> section = Section::makeSection(s, dataVersion);
		size_t Y = section->getY();
		sections.resize(Y);
		sections.push_back(std::move(section));
	}
}

uint8_t Chunk::getBiome(size_t x, size_t y, size_t z) const {
	if (x > SIZE || y > MAXY || z > SIZE)
		throw std::invalid_argument("corrupt chunk data");

	if (biomeInts)
		return biomeInts->getValue((y/BGROUP)*BSIZE*BSIZE + (z/BGROUP)*BSIZE + (x/BGROUP));
	else if (biomeIntsPre115)
		return biomeIntsPre115->getValue(z*SIZE + x);
	else if (biomeBytes)
		return biomeBytes->getValue(z*SIZE + x);
	else
		return 0xff;
}

Block Chunk::getBlock(size_t x, Chunk::Height height, size_t z) const {
	Block block = {};

	block.depth = height.depth;

	size_t Y = height.y / SIZE;
	size_t y = height.y % SIZE;

	if (Y < sections.size() && sections[Y])
		block.type = sections[Y]->getBlockStateAt(x, y, z);

	size_t Yt = (height.y + 1) / SIZE;
	size_t yt = (height.y + 1) % SIZE;

	if (Yt < sections.size() && sections[Yt])
		block.blockLight = sections[Yt]->getBlockLightAt(x, yt, z);

	return block;
}

bool Chunk::getHeight(Chunk::Height *height, const Section *section, size_t x, size_t y, size_t z, int flags) const {
	if (height->depth > 0)
		return false;

	if (!(flags & WITH_DEPTH) && height->y > 0)
		return false;

	const Resource::BlockType *type = section->getBlockStateAt(x, y, z);
	if (!type || !(type->flags & BLOCK_OPAQUE))
		return false;

	if (height->y == 0)
		height->y = SIZE*section->getY() + y;

	if (!(flags & WITH_DEPTH))
		return true;

	if (type->flags & BLOCK_WATER)
		return false;

	height->depth = SIZE*section->getY() + y;

	return true;
}

Chunk::Heightmap Chunk::getTopLayer(int flags) const {
	size_t done = 0;
	Heightmap ret = {};

	for (ssize_t Y = sections.size() - 1; Y >= 0; Y--) {
		if (done == SIZE*SIZE)
			break;

		if (!sections[Y])
			continue;

		const Section *section = sections[Y].get();

		for (ssize_t y = SIZE-1; y >= 0; y--) {
			if (done == SIZE*SIZE)
				break;

			for (size_t z = 0; z < SIZE; z++) {
				for (size_t x = 0; x < SIZE; x++) {
					if (getHeight(&ret.v[x][z], section, x, y, z, flags))
						done++;
				}
			}
		}
	}

	return ret;
}

}
}
