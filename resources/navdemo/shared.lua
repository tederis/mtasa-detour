TILE_SIZE = 16
TILES_NUM = math.ceil( 6000 / TILE_SIZE )

WORLD_SIZE = TILE_SIZE * TILES_NUM
WORLD_MIN_X = -WORLD_SIZE / 2
WORLD_MIN_Y = -WORLD_SIZE / 2

DEBUG_MODE_NAMES = {
    "Disabled",
    "Navmesh",
    "Colmesh"
}

DEBUG_MODE_DISABLED = 1
DEBUG_MODE_NAVMESH = 2
DEBUG_MODE_COLMESH = 3

function getWorldTile( x, y )
    local tx = math.floor((x - WORLD_MIN_X) / TILE_SIZE)
    local ty = math.floor((y - WORLD_MIN_Y) / TILE_SIZE)

    return tx, ty
end

function getWorldTileIndex( tx, ty )
    return ty * TILES_NUM + tx
end

function getWorldTileFromIndex( index )
    return index % TILES_NUM, math.floor( index / TILES_NUM )
end

function getTileBounds( index )
    local tx, ty = getWorldTileFromIndex( index )
    local tileX, tileY = WORLD_MIN_X + tx*TILE_SIZE, WORLD_MIN_Y + ty*TILE_SIZE
    return tileX, tileY, tileX + TILE_SIZE, tileY + TILE_SIZE
end
