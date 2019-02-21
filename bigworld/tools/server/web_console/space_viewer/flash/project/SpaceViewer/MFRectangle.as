package
{
	public class MFRectangle
	{
		public var minX:Number=0;
		public var maxX:Number=0;
		public var minY:Number=0;
		public var maxY:Number=0;
		public function MFRectangle(x1:Number,y1:Number,x2:Number,y2:Number):void
		{
			minX = Math.min( x1, x2 );
			maxX = Math.max( x1, x2 );
			minY = Math.min( y1, y2 );
			maxY = Math.max( y1, y2 );
		}
		public function toString():String
		{
			return minX.toString()+" "+minY.toString()+" "+maxX.toString()+"  "+maxY.toString();
		}
		public function deepCopy():MFRectangle
		{
			var temp:MFRectangle=new MFRectangle(this.minX,this.minY,this.maxX,this.maxY);
			return temp;
		}
	}
}