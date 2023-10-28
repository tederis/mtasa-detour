addEventHandler( "onClientPreRender", root,
    function( dt )
        for _, ped in ipairs( getElementsByType( "ped", root, true ) ) do
            local path = getElementData( ped, "path", false )
            if path then
                processPedPath( ped, path )
            end
        end
    end
, false )

function utilGetNearestPoint( x, y, z, x0, y0, z0, x1, y1, z1 )
    local dx = x1 - x0
    local dy = y1 - y0
    local dz = z1 - z0
    local px = x - x0
    local py = y - y0
    local pz = z - z0

    local k = ( dx*px + dy*py + dz*pz ) / ( dx*dx + dy*dy + dz*dz )
    k = math.max( math.min( k, 1 ), 0 )

    return k*x1 + (1-k)*x0, k*y1 + (1-k)*y0, k*z1 + (1-k)*z0, k
end

function utilGetPathLen( path )
    local len = 0

    if #path < 2 then
        outputDebugString( "Invalid index", 2 )
        return 0
    end

    for i = 1, #path - 1 do
        local p0 = path[ i ]
        local p1 = path[ i + 1 ]

        len = len + math.sqrt( 
            math.pow( p0[ 1 ] - p1[ 1 ], 2 ) + math.pow( p0[ 2 ] - p1[ 2 ], 2 ) + math.pow( p0[ 3 ] - p1[ 3 ], 2 ) 
        )
    end

    return len
end

function utilGetPathLenTill( path, index )
    local len = 0

    if #path < 2 or index <= 1 or index > #path then
        return 0
    end

    for i = 1, index - 1 do
        local p0 = path[ i ]
        local p1 = path[ i + 1 ]

        len = len + math.sqrt( 
            math.pow( p0[ 1 ] - p1[ 1 ], 2 ) + math.pow( p0[ 2 ] - p1[ 2 ], 2 ) + math.pow( p0[ 3 ] - p1[ 3 ], 2 ) 
        )
    end

    return len
end

function utilGetPathSegmentLen( path, segmentIndex )
    if #path < 2 or segmentIndex < 1 or segmentIndex >= #path then
        outputDebugString( "Invalid index", 2 )
        return 0
    end

    local p0 = path[ segmentIndex ]
    local p1 = path[ segmentIndex + 1 ]

    return math.sqrt( 
        math.pow( p0[ 1 ] - p1[ 1 ], 2 ) + math.pow( p0[ 2 ] - p1[ 2 ], 2 ) + math.pow( p0[ 3 ] - p1[ 3 ], 2 ) 
    )
end

function utilGetPathTime( path, x, y, z )
    local nearestIndex = nil
    local nearestDistSqr = nil
    local nearestTime = nil

    for i = 1, #path - 1 do
        local p0 = path[ i ]
        local p1 = path[ i + 1 ]

        local px, py, pz, pk = utilGetNearestPoint( x, y, z, p0[ 1 ], p0[ 2 ], p0[ 3 ], p1[ 1 ], p1[ 2 ], p1[ 3 ] )
        local distSqr = math.pow( px - x, 2 ) + math.pow( py - y, 2 ) + math.pow( pz - z, 2 )
        if not nearestIndex or distSqr < nearestDistSqr then
            nearestIndex = i
            nearestDistSqr = distSqr
            nearestTime = pk
        end
    end

    if nearestIndex then
        local pathLen = utilGetPathLen( path )
        local pathLenTill = utilGetPathLenTill( path, nearestIndex )

        local t = pathLenTill / pathLen + ( nearestTime * utilGetPathSegmentLen( path, nearestIndex ) ) / pathLen

        local p0 = path[ nearestIndex ]
        local p1 = path[ nearestIndex + 1 ]

        return t
    end

    outputDebugString( "Nearest point was not found", 2 )
    return 0
end

function utilGetPathPoint( path, t )
    local len = 0
    local pt = 0

    if #path < 2 then
        outputDebugString( "Invalid index", 2 )
        return false
    end

    local pathLen = utilGetPathLen( path )

    for i = 1, #path - 1 do
        local p0 = path[ i ]
        local p1 = path[ i + 1 ]

        len = len + math.sqrt( 
            math.pow( p0[ 1 ] - p1[ 1 ], 2 ) + math.pow( p0[ 2 ] - p1[ 2 ], 2 ) + math.pow( p0[ 3 ] - p1[ 3 ], 2 ) 
        )

        local nt = len / pathLen
        if t > pt and t <= nt then
            local x0, y0, z0, x1, y1, z1 = p0[ 1 ], p0[ 2 ], p0[ 3 ], p1[ 1 ], p1[ 2 ], p1[ 3 ]
            local k = math.unlerp( t, pt, nt )

            return k*x1 + (1-k)*x0, k*y1 + (1-k)*y0, k*z1 + (1-k)*z0, k
        end

        pt = nt
    end

    return false
end

function processPedPath( ped, path )
    local x, y, z = getElementPosition( ped )

    local endNode = path[ #path ]
    local endX, endY, endZ = endNode[ 1 ], endNode[ 2 ], endNode[ 3 ]
    local distSqr = math.pow( endX - x, 2 ) + math.pow( endY - y, 2 ) + math.pow( endZ - z, 2 )
    if distSqr < 2 then
        setPedControlState( ped, "forwards", false )
        return
    end

    local pathLen = utilGetPathLen( path )

    local t = utilGetPathTime( path, x, y, z )
    local d = t * pathLen
    local nextD = d + 2
    local nextT = math.clamp( 0, 1, nextD / pathLen )

    local nx, ny, nz = utilGetPathPoint( path, nextT )
    if nx then        
        setPedControlState( ped, "forwards", true )

        local dx, dy = nx - x, ny - y
        local rot = math.atan2( dx, dy )
        setElementRotation( ped, 0, 0, 360 - math.deg( rot ), "default", true )
    end
end