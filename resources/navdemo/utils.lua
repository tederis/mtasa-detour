function math.clamp ( lower, upper, value )
	return math.max ( math.min ( value, upper ), lower )
end

function math.lerp(a, b, k)
	return a * (1-k) + b * k
end

function math.unlerp( v, a, b )
	return math.max( math.min( ( v - a ) / ( b - a ), 1 ), 0 )
end

function math.unlerpclamped( from, to, pos )
	return math.clamp( 0, 1, math.unlerp( pos, from, to ) )
end