local sw, sh = guiGetScreenSize()

local function drawBounds( minX, minY, maxX, maxY, z, color )
	dxDrawLine3D( minX, minY, z, maxX, minY, z, color )
	dxDrawLine3D( minX, minY, z, minX, maxY, z, color )
	dxDrawLine3D( minX, maxY, z, maxX, maxY, z, color )
	dxDrawLine3D( maxX, maxY, z, maxX, minY, z, color )
end

local function drawCenteredText( text, y )
	dxDrawText( text, 0, y, sw, sh, tocolor( 255, 255, 255 ), 1.2, "default", "center" )
end

local REVISION = -1
local CNUNKS
local MODE = DEBUG_MODE_DISABLED

local TILE_DATA_STATE = {

}

local DOTS = {
	".",
	"..",
	"..."
}

function getTileTransferState()
	return next( TILE_DATA_STATE ) ~= nil
end

addEventHandler( "onClientRender", root,
	function( dt )
		local textY = 30
		drawCenteredText( "Press Q to change the debug mode", textY )
		textY = textY + 30		

		drawCenteredText( "Press X to make a ped following you", textY )
		textY = textY + 30

		drawCenteredText( "Press E to respawn a ped", textY )
		textY = textY + 30

		if getTileTransferState() then
			local idx = math.floor( ( getTickCount() / 1000 ) % 3 ) + 1
			drawCenteredText( "Data transferring in progress" .. DOTS[idx], textY )
		else
			drawCenteredText( "Debug mode: " .. DEBUG_MODE_NAMES[MODE], textY )
		end


		local x, y, z = getElementPosition( localPlayer )
		local tx, ty = getWorldTile( x, y )
        local index = getWorldTileIndex( tx, ty )
		
		local minX, minY, maxX, maxY = getTileBounds( index )
		drawBounds( minX, minY, maxX, maxY, z, tocolor( 255, 0, 0 ) )

		if type( CNUNKS ) == "table" then
			for _, chunk in ipairs( CNUNKS ) do
				dxDrawPrimitive3D( "trianglelist", false, unpack( chunk ) )

				local verticesNum = #chunk
				assert(verticesNum % 3 == 0)

				local trisNum = verticesNum / 3
				for i = 1, trisNum do
					local v1 = chunk[ (i - 1) * 3 + 1 ]
					local v2 = chunk[ (i - 1) * 3 + 2 ]
					local v3 = chunk[ (i - 1) * 3 + 3 ]

					local lineColor = tocolor( 0, 255, 0 )
					local lineWidth = 1.5
					dxDrawLine3D( v1[ 1 ], v1[ 2 ], v1[ 3 ], v2[ 1 ], v2[ 2 ], v2[ 3 ], lineColor, lineWidth )
					dxDrawLine3D( v3[ 1 ], v3[ 2 ], v3[ 3 ], v2[ 1 ], v2[ 2 ], v2[ 3 ], lineColor, lineWidth )
					dxDrawLine3D( v3[ 1 ], v3[ 2 ], v3[ 3 ], v1[ 1 ], v1[ 2 ], v1[ 3 ], lineColor, lineWidth )
				end
			end		
		end		
    end
, false )

addEvent( "onDebugTileDataState", true )
addEventHandler( "onDebugTileDataState", resourceRoot,
	function( revision, mode )
		if mode ~= DEBUG_MODE_DISABLED then
			TILE_DATA_STATE[ revision ] = true
		end
		
		MODE = mode
	end
, false )

addEvent( "onDebugTileData", true )
addEventHandler( "onDebugTileData", resourceRoot,
	function( revision, vertices )
		TILE_DATA_STATE[ revision ] = nil

		if revision <= REVISION then
			return
		end

		REVISION = revision
		CNUNKS = {}

		local chunkSize = 900

		local chunksNum = math.ceil( #vertices / chunkSize )
		for i = 1, chunksNum do
			local chunk = {}

			local first = (i-1) * chunkSize
			local n = math.min( #vertices - first, chunkSize )

			for j = 1, n do
				local vert = vertices[ first + j ]
				vert[ 4 ] = tocolor( 255, 0, 25, 50 )
				table.insert( chunk, vert )
			end

			table.insert( CNUNKS, chunk )
		end
	end
, false )