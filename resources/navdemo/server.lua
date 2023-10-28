
function rebuildNavmesh( createDump )
    outputDebugString( "Start navigation mesh building..." )
    navBuild()
    
    outputDebugString( "Start navigation mesh saving..." )
    navSave( "world.bin" )

    if createDump then
        outputDebugString( "Start navigation mesh dumping..." )
        navDump( "dump.obj" )
    end
end

addEventHandler("onResourceStart", resourceRoot,
    function()
        outputDebugString( "Start navigation mesh loading..." )

        if navState() then
            outputDebugString( "World navigation mesh is already loaded" )
            return
        end

        if navLoad( "world.bin") then
            outputDebugString( "World navigation mesh is successfully loaded")     
        else
            rebuildNavmesh()
        end     
    end
, false )

addCommandHandler( "rebuildnav", function( )
    rebuildNavmesh()
end )

addCommandHandler( "dumpnav", function( )
    outputDebugString( "Start navigation mesh dumping..." )
    navDump( "dump.obj" )
end )

local PLAYER_DATA = {

}

function createPlayerData()
    local data = {
        index = -1, -- Cell index
        mode = DEBUG_MODE_DISABLED,
        revision = 0
    }
    return data
end

function updatePlayerDebugTile( player, data )
    local minX, minY, maxX, maxY = getTileBounds( data.index )
    local bias = 50

    local vertices
    if data.mode == DEBUG_MODE_NAVMESH then
        vertices = navNavigationMesh( minX - bias, minY - bias, 0, maxX + bias, maxY + bias, 0, 0.0 )
    elseif data.mode == DEBUG_MODE_COLMESH then
        vertices = navCollisionMesh( minX - bias, minY - bias, 0, maxX + bias, maxY + bias, 0, 0.1 )
    else
        vertices = {}
    end    

    triggerClientEvent( player, "onDebugTileDataState", resourceRoot, data.revision, data.mode )
    triggerLatentClientEvent( player, "onDebugTileData", resourceRoot, data.revision, vertices )
 
    data.revision = data.revision + 1
end

function updatePlayerData( player, data )
    local x, y = getElementPosition( player )
    local tx, ty = getWorldTile( x, y )
    local index = getWorldTileIndex( tx, ty )
    if data.index ~= index then
        data.index = index
        updatePlayerDebugTile( player, data )
    end
end

setTimer( function()
    for _, player in ipairs( getElementsByType( "player" ) ) do
        local data = PLAYER_DATA[player]
        if data then
            updatePlayerData( player, data )
        end        
    end    
end, 1000, 0 )

local function onPlayerDebugKey( player, key )
    local data = PLAYER_DATA[player]
    if not data then
        return
    end

    data.mode = data.mode + 1
    if data.mode > DEBUG_MODE_COLMESH then
        data.mode = 1
    end
    outputChatBox( "New debug mode: " .. DEBUG_MODE_NAMES[ data.mode ] )

    if data.index >= 0 then
        updatePlayerDebugTile( player, data )
    end
end

function addPlayer( player )
    PLAYER_DATA[ player ] = createPlayerData()

    bindKey( player, "q", "down", onPlayerDebugKey )
end

function removePlayer( player )
    PLAYER_DATA[ player ] = nil

    unbindKey( player, "q", "down", onPlayerDebugKey )
end

addEventHandler( "onPlayerQuit", root,
    function()
        removePlayer( source )
    end
)

addEventHandler( "onPlayerJoin", root,
    function()
        addPlayer( source )
    end
)

addEventHandler( "onResourceStart", resourceRoot,
    function()
        for _, player in ipairs( getElementsByType( "player" ) ) do
            addPlayer( player )
        end
    end
)

addEvent( "onFindNavmeshPath", true )
addEventHandler( "onFindNavmeshPath", resourceRoot,
    function ( bx, by, bz, x, y, z )
        local path = navFindPath( bx, by, bz, x, y, z )
        if path then
            triggerClientEvent( client, "onClientFindNearestPoint", resourceRoot, path )
        end
    end
, false )