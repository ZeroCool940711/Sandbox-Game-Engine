package
{
	import flash.utils.Dictionary;
	
	public class LeafNode extends BranchNode
	{
		public var appID:int=0;
		public function LeafNode(appID_:int,nodeType_temp:int):void
		{
			//BranchNode(nodeType_temp);
			appID=appID_;
			nodeType=nodeType_temp;
		}	
		override public function visit(rect:MFRectangle, listOfPoints:Array, cellRects:Dictionary):void	
		{
			cellRects[appID]=rect;	
		}
	}
}