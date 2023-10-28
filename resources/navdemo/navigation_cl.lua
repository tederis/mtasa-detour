local worldMarkerPos = nil
local nearestMarkerPos = nil
local pathMarkers = nil

local beginMarker = nil
local endMarker = nil

local ped = nil
local followMeTimer = nil

local function drawAxis( vec, clr )
    dxDrawLine3D( vec, vec + Vector3( 1, 0, 0 ), clr, 3 )
    dxDrawLine3D( vec, vec + Vector3( 0, 1, 0 ), clr, 3 )
    dxDrawLine3D( vec, vec + Vector3( 0, 0, 1 ), clr, 3 )
end

local function drawPath( path, clr )
    if type( path ) ~= "table" or #path < 2 then
        return
    end    

    local bias = 0

    for i = 1, #path - 1 do
        local v0 = path[ i ]
        local v1 = path[ i + 1 ]   

        dxDrawLine3D( Vector3( v0[ 1 ], v0[ 2 ], v0[ 3 ] + bias ), Vector3( v1[ 1 ], v1[ 2 ], v1[ 3 ] + bias ), clr, 10 )
    end
end

local function onRender()
    local DRAW = true
    if DRAW then
        if beginMarker then
            dxDrawWiredSphere( beginMarker, 0.15, tocolor( 255, 0, 0 ), 2, 1 )
        end

        if endMarker then
            dxDrawWiredSphere( endMarker, 0.15, tocolor( 0, 255, 0 ), 2, 1 )
        end

        if pathMarkers then
            drawPath( pathMarkers, tocolor( 255, 255, 0 ) )
        end
    end 

    if pathMarkers then
        processPedPath( ped, pathMarkers )
    end
end

local function onClick( button, state, absoluteX, absoluteY, worldX, worldY, worldZ )
    if state ~= "down" then
        return
    end

    local x, y, z = getElementPosition( ped )
    beginMarker = Vector3( x, y, z  )

    if button == "left" then
        
    else
        endMarker = Vector3( worldX, worldY, worldZ )
    end

    if beginMarker and endMarker then
        triggerServerEvent( "onFindNavmeshPath", resourceRoot, beginMarker.x, beginMarker.y, beginMarker.z, endMarker.x, endMarker.y, endMarker.z )
    end
end

local function toggleMouse()
    showCursor( not isCursorShowing() )
end

addEvent( "onClientFindNearestPoint", true )
addEventHandler( "onClientFindNearestPoint", resourceRoot, 
    function( path )
        pathMarkers = path
    end
, false )

local function followFn()
    local x, y, z = getElementPosition( ped )
    beginMarker = Vector3( x, y, z  )

    local px, py, pz = getElementPosition( localPlayer )
    endMarker = Vector3( px, py, pz )

    if beginMarker and endMarker then
        triggerServerEvent( "onFindNavmeshPath", resourceRoot, beginMarker.x, beginMarker.y, beginMarker.z, endMarker.x, endMarker.y, endMarker.z )
    end
end

function toggleFollowTicker()
    if isTimer( followMeTimer ) then
        killTimer( followMeTimer )
        followMeTimer = nil
    else
        followMeTimer = setTimer( followFn, 100, 0 )
    end  
end

function respawnPed()
    local x, y, z = getElementPosition( localPlayer )
    if isElement( ped ) then
        setElementPosition( ped, x, y + 3, z )
    else        
        ped = createPed( 31, x, y + 3, z )
    end
end

addEventHandler( "onClientResourceStart", resourceRoot,
    function()
        addEventHandler( "onClientRender", root, onRender, false )
        addEventHandler( "onClientClick", root, onClick, false )

        bindKey( "z", "down", toggleMouse )
        bindKey( "e", "down", respawnPed )
        bindKey( "x", "down", toggleFollowTicker )
        
        respawnPed()

        outputChatBox( "Enter /rebuildnav to rebuild a navmesh" )
        outputChatBox( "Enter /dumpnav to make a navmesh dump" )
    end
, false )

