// SPDX-License-Identifier: BSD-2-Clause
/*
  Copyright (c) 2015, Matthias Schiffer <mschiffer@universe-factory.net>
  All rights reserved.
*/


#pragma once

#include "../NBT/CompoundTag.hpp"
#include "../Resource/Biome.hpp"
#include "../Resource/BlockType.hpp"


namespace MinedMap {
namespace World {

struct Block {
	const Resource::BlockType *type;
	unsigned depth;
	uint8_t blockLight;

	bool isVisible() const {
		return type && (type->flags & BLOCK_OPAQUE);
	}

	Resource::FloatColor getColor(uint8_t biome) const {
		if (!isVisible())
			return Resource::FloatColor {};

		return (Resource::BIOMES[biome] ?: Resource::BIOME_DEFAULT)->getBlockColor(type, depth);
	}

	operator bool() const {
		return type;
	}
};

}
}
