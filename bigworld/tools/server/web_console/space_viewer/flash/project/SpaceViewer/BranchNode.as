package
{
	import flash.geom.Point;
	import mx.controls.Alert;
	import flash.utils.Dictionary;
		
	public class BranchNode
	{
		public var position:Number =0;
		//public var load:Number = 0;
		public var left:BranchNode = null;
		public var right:BranchNode=null;
		public var isHorizontal:Boolean = false;
		public var nodeType:int=0;
		
		public function BranchNode():void
		{
			
		}
		public function setData(position_temp:Number,nodeType_temp:int):void
		{
			position = position_temp;
			//load	 = load_temp;
			left	 = null;
			right	 = null;
			nodeType=nodeType_temp;
			if(nodeType==0)
				isHorizontal = true;
			else
				isHorizontal = false;
			
		}
		public function visit(rect:MFRectangle, listOfPoints:Array ,cellRects:Dictionary):void
		{
			//Alert.show("came in to vist  "+nodeType.toString());
			if(nodeType==0 || nodeType==1)
			{
			var leftRect:MFRectangle = rect.deepCopy();
			var rightRect:MFRectangle = rect.deepCopy();
			var pt1:Point;
			var pt2:Point;
			if(isHorizontal==true)
			{
				//Alert.show(isHorizontal.toString());
				leftRect.maxY = position;
				rightRect.minY = position;
				pt1 = new Point(rect.minX,position);
				pt2 = new Point(rect.maxX,position);
			}
			else
			{
				//Alert.show(isHorizontal.toString());
				leftRect.maxX = position;
				rightRect.minX = position;
				pt1 = new Point(position, rect.minY);
				pt2 = new Point(position, rect.maxY);
			}
			
				listOfPoints.push(pt1);
				listOfPoints.push(pt2);
				if(left)
					left.visit(leftRect,listOfPoints,cellRects);
				if(right)
					right.visit(rightRect,listOfPoints,cellRects);
			}
			if(nodeType==2)
			{
				//Alert.show(rect.toString());
											
			}		
		}
	}
}