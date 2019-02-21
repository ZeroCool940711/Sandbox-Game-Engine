package
{
	/**
	 * This class can be used to draw a dashed line, as there is no native support in Flex. 
	 */	
	public class DashedLine
	{
		private var isHoriZontal:Boolean = false;
		private var isVertical:Boolean   = false;
			
		/**
		 * This function draws the dashedLine on the target object
		 */		
		public function drawDashedLine(target:*, x1:int, y1:int, x2:int, y2:int, lineLength:uint, lineThickness:uint, lineColor:uint):void
		{
			var complete:Boolean = false;
			target.graphics.lineStyle(lineThickness, lineColor, 1);			
			var tempVal:uint = 0;
			if (y1 == y2)
			{
				isHoriZontal=true;
				if (x1 > x2)
				{
					tempVal = x1;
					x1 		= x2;
					x2 		= tempVal;					
				}				
			}
			else
			{
				isVertical=true;
				if (y1 > y2)
				{
					tempVal = y1;
					y1 		= y2;
					y2 		= tempVal;
				}				
			}
			if (isHoriZontal)
			{
				while(x1<=(x2-lineLength))
				{
					target.graphics.moveTo(x1 , y1);
					target.graphics.lineTo(x1 + lineLength, y1);
					x1 = x1+2*lineLength;
				}
				if(x1<=x2)
				{
					target.graphics.moveTo(x1 , y1);
					target.graphics.lineTo(x2, y1);
				}
				isHoriZontal = false;
			}
			if(isVertical)
			{
				while(y1<=(y2-lineLength))
				{
					target.graphics.moveTo(x1 , y1);
					target.graphics.lineTo(x1 , y1+lineLength);
					y1 = y1+2*lineLength;
				}
				if(y1<=y2)
				{
					target.graphics.moveTo(x1 , y1);
					target.graphics.lineTo(x1, y2);
				}
				isVertical = false;
			}
		}
	}
}