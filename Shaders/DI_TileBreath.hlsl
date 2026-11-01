// Custom node body for M_Tile's World Position Offset (vertex shader). Inputs: Kind, Seed (0..1 per piece), Time.
// Output: world offset. Each stone drifts ~1.3 units (about 1.5 px) on its own slow rhythm, so the board feels
// alive without the CPU touching instances. Aim dots (4) and ripples (5) stay put.
if (round(Kind) >= 4)
{
	return float3(0, 0, 0);
}
float Phase = Seed * 6.283185;
float2 Board = float2(sin(Time * 0.8 + Phase), cos(Time * 0.63 + Phase * 1.7)) * 2.2;
return float3(Board.y, Board.x, 0); // board X right = world Y, board Y up = world X
