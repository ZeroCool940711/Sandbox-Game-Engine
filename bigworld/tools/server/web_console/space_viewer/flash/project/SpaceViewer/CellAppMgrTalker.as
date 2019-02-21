package
{
	import mx.controls.Alert;
	import flash.utils.Dictionary;
	
	
	public class CellAppMgrTalker
	{
		public var tree:BranchNode;
		
		public function CellAppMgrTalker(list:Array):void
		{
			try
			{
				//Alert.show(list.length.toString());
			var stack:Array = new Array();
			tree = null;
			var i:int=0;
			var nodeType:int=0;
			var shouldAppend:Boolean=false;
			var newNode:BranchNode=null;
			while( (stack.length > 0) || (tree==null) || i < list.length)
			{
				nodeType=list[i].nodeType;
				shouldAppend = false;
				// Leaf nodes (i.e. cellapps)
				if (nodeType == 2)
				{
					// Create object for cell and map it in
					//Alert.show("leaf node "+list[i].appID);
					newNode = new LeafNode(list[i].appID,list[i].nodeType);
				}
				else if(nodeType == 0 || nodeType == 1)
				{
					// Branch nodes
					newNode = new BranchNode();
					//Alert.show("branch node "+list[i].position.toString());
					newNode.setData( list[i].position , list[i].nodeType);
					shouldAppend = true;
				}
				else
					newNode = null;
				
				if (stack.length > 0)
				{
					if (stack[stack.length-1].left == null)
					{
						stack[stack.length-1].left = newNode;
					}
					else
					{
						stack[stack.length-1].right = newNode;
						stack.pop();
					}
				}
				else
				{
					//Alert.show((tree==null).toString());
					tree = newNode;
				}
				if(shouldAppend==true)
					stack.push( newNode );
				
				i++;
			}
			}
			catch(e:Error)
			{
				
			}
		}
		public function visit(rect:MFRectangle, listOfPoints:Array ,cellRects:Dictionary):void
		{
			try
			{
			tree.visit(rect,listOfPoints,cellRects);		
			}
			catch(e:Error)
			{
				
			}
		}
		public function onDestroy():void
		{
			
		}
		
	}
}