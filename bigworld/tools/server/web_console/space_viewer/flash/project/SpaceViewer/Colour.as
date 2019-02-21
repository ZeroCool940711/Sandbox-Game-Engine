/**
 * Colour utilities.
 */
package
{
	public class Colour
	{
		// HSB to RGB converter 
		// http://www.easyrgb.com/math.php?MATH=M21#text21
		public function HSV2RGB( H:Number, S:Number, V:Number ):uint
		{
			var R:Number;
			var G:Number;
			var B:Number;
			var var_r:Number;
			var var_g:Number;
			var var_b:Number;
			
			if(S == 0)
			{
				R = V * 255.0;
				G = V * 255.0;
				B = V * 255.0;
			}
			else
			{
				var var_h:Number = H * 6;
				if(var_h == 6.0)
				{
					var_h = 0.0;
				} 
				var var_i:int    = int( var_h );
				var var_1:Number = V * ( 1 - S );
				var var_2:Number = V * ( 1 - S * ( var_h - var_i ) );
				var var_3:Number = V * ( 1 - S * ( 1 - ( var_h - var_i ) ) );
			
				if(var_i == 0)
				{
					var_r = V;
					var_g = var_3;
					var_b = var_1;
				}
				else if(var_i == 1)
				{
					var_r = var_2;
					var_g = V;
					var_b = var_1;
				}
				else if(var_i == 2)
				{
					var_r = var_1;
					var_g = V;
					var_b = var_3;
				}
				else if(var_i == 3)
				{
					var_r = var_1;
					var_g = var_2;
					var_b = V;
				}
				else if(var_i == 4)
				{
					var_r = var_3;
					var_g = var_1;
					var_b = V;
				}
				else
				{
					var_r = V;
					var_g = var_1;
					var_b = var_2;
				}
				R = var_r * 255.0;
				G = var_g * 255.0;
				B = var_b * 255.0;
			}
			var colour:uint = uint(R*Math.pow(16,4)+G*Math.pow(16,2)+B*Math.pow(16,0));
			return colour;
		}	
	}
}