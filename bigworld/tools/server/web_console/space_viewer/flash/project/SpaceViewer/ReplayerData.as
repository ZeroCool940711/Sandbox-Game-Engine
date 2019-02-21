package
{	
	import flash.utils.Timer;
	import flash.events.TimerEvent;
	import mx.controls.Alert;
	import mx.rpc.events.ResultEvent;
	import com.adobe.serialization.json.JSON;
	import mx.rpc.http.mxml.HTTPService;
	import mx.core.Application;
	import mx.rpc.events.FaultEvent;
	//import mx.controls.Label;	
	
	/**
	 *  This class consists of all the data required to draw a space. It gets 
	 * 	data repeatedly from server after specified time interval.
	 */
	 
	public class ReplayerData
	{
		private var space_Bounds:Array;
		private var grid_Resolution:Number;
		private var ghost_EntityData:Array;
		private var entity_Data:Array;
		private var cell_Boundary:Array;
		private var chunk_Bounds:Array;
		
		//timer to make request every time interval 
		private var myTimer:Timer;
		private var httpService:HTTPService;
		private var httpForCellSelect:HTTPService;
		
		//to keep track of uid and space 
		private var UID:String;
		private var space:String;
		
		private var request:Object;
		private var requestForCell:Object;
		private var serviceURL:String;
		private var serviceURLForCell:String;
		private var transimage:Transformation;
		public var spaceGeometry:Object;
		public var cellRect:Array;
		public var cellRects:Array;
		public var cellDataIds:Array;
		public var cellDataIps:Array;
		public var cellDataLoads:Array;
		public var typeNames:Object;
		public var entityBounds:Array;
		public var branchLoads:Array;
		
		public var cellList:Array;
		public var cmt:CellAppMgrTalker=null;
		//private var lab:Label=new Label();
		

		public function get cellBoundary():Array
		{
			return cell_Boundary;
		}
		public function get spaceBounds():Array
		{
			return space_Bounds;
		}
		public function get chunkBounds():Array
		{
			return chunk_Bounds;
		}
		public function get gridResolution():Number
		{
			return grid_Resolution;
		}
		public function get ghostEntityData():Array
		{
			return ghost_EntityData;
		}
		public function get entityData():Array
		{
			return entity_Data;
		}
		
		public function ReplayerData(username:String,spaceId:String,transimagetemp:Transformation):void
		{
			UID=username;
			//Application.application.addChild(lab);
			transimage=transimagetemp;
			space=spaceId;
			request=new Object();
			request.UID=UID;
			request.space=space;
			request.rand=Math.random();
			//this gives server address we have to set flash varaible in kid
			serviceURL=Application.application.parameters.serviceURL;
			if(serviceURL=="undefined")
			{
				Alert.show("No url got from kid in replayerData","Error");
				return;
			}
			serviceURL+="/sv/getData?tg_format=json";
			httpService=new HTTPService();
			httpService.url=serviceURL;
			httpService.addEventListener(FaultEvent.FAULT,onFault);
			
			httpForCellSelect=new HTTPService();
			serviceURLForCell=Application.application.parameters.serviceURL;
			if(serviceURLForCell=="undefined")
			{
				Alert.show("No url got from kid in replayerData for selecting cell","Error");
				return;
			}
			serviceURLForCell+="/sv/selectCell?tg_format=json";
			httpForCellSelect.url=serviceURLForCell;
			httpForCellSelect.showBusyCursor=true;
			httpForCellSelect.addEventListener(FaultEvent.FAULT,onFaultCellSelecting);
			httpService.addEventListener(ResultEvent.RESULT,onResult);
			try
			{
				request.rand=Math.random();
				request.x1=transimage.iTransformX(-3);
				request.y1=transimage.iTransformY(transimage.height+3);
				request.x2=transimage.iTransformX(transimage.width+3);
				request.y2=transimage.iTransformY(-3);
				httpService.send(request);
			}
			catch(err:Error)
			{
				//Alert.show("error in request to server in replayerData of getData"+err.message);
			}
			httpForCellSelect.addEventListener(ResultEvent.RESULT,onCellResult);
			myTimer=new Timer(1000);
			myTimer.start();
			myTimer.addEventListener(TimerEvent.TIMER,timerHandler);			
		}
		/**
		 * This is handler for timer for each tick event it sends a request for
		 * getting space data
		 */
		public function timerHandler(event:TimerEvent=null):void
		{
			try
			{
				request.rand=Math.random();
				request.x1=transimage.iTransformX(-3);
				request.y1=transimage.iTransformY(transimage.height+3);
				request.x2=transimage.iTransformX(transimage.width+3);
				request.y2=transimage.iTransformY(-3);
				
				//lab.text="x1:"+request.x1.toString()+" y1:"+request.y1.toString()+" before : "+(transimage.height+3).toString();
				httpService.send(request);
			}
			catch(err:Error)
			{
				//Alert.show("error in request to server in replayer for get data "+err.message);
				return;
			}
		}
				
		/**
		 * This is handler for httpservice result of selecting cell
		 * it consists of sending the getspaces request to get updated information
		 */
		 
		public function onCellResult(event:ResultEvent):void
		{
			try
			{
				request.rand=Math.random();
				request.x1=transimage.iTransformX(-3);
				request.y1=transimage.iTransformY(transimage.height+3);
				request.x2=transimage.iTransformX(transimage.width+3);
				request.y2=transimage.iTransformY(-3);
				httpService.send(request);
			}
			catch(err:Error)
			{
				//Alert.show("error in request to server in replayer for selectCell"+err.message);
			}	
			myTimer.start();			
		}
		/**
		 * This is handler for getData Httpservice request. Consists of getting the latest 
		 * data and assigning then to variables.
		 */
		public function onResult(event:ResultEvent):void
		{
			try
			{
				var decoded:Object=JSON.decode(httpService.lastResult.toString());
				space_Bounds=decoded.spaceBounds;
				grid_Resolution=decoded.gridResolution;
				ghost_EntityData=decoded.ghostEntityData;
				entity_Data=decoded.entityData;
				cell_Boundary = decoded.cellBoundary;
				cellRect=decoded.cellRect;
				cellRects=decoded.cellRects;
				cellDataIps=decoded.cellDataIps;
				cellDataIds=decoded.cellDataIds;	
				cellDataLoads=decoded.cellLoads;		
				typeNames=decoded.typeNames;
				entityBounds=decoded.entityBoundsList;	
				branchLoads=decoded.branchLoads;
				spaceGeometry=decoded.spaceGoemetry;
				chunk_Bounds=decoded.chunkBounds;
				cellList=decoded.cellList;
				//if(cmt==null)
				//{
				cmt=new CellAppMgrTalker(cellList);						
				//}
				//Release the memory		
				decoded	= null;
			}
			catch(err:Error)
			{
				//Alert.show("error occured in response in replayer class "+err.message);
			}
		}
		/**
		 * This is to stopTimer it means stop sending requests for new data
		 */
		public function stopTimer():void
		{
			myTimer.stop();
		}
		/**
		 * This is to startTimer it means start sending requests for new data
		 */
		public function startTimer():void
		{
			myTimer.start();
		}
		public function set Delay(time:int):void
		{
			myTimer.delay=time;
		}
		public function get Delay():int
		{
			return myTimer.delay;
		}
		/**
		 * This function sends a request to server to select cell for viewer at a
		 * particular point
		 */
		public function changeCell(x:Number,y:Number):void
		{
			myTimer.stop();
			requestForCell=new Object();
			requestForCell.UID=UID;
			requestForCell.space=space;
			requestForCell.x=x;
			requestForCell.y=y;
			requestForCell.rand=Math.random();
			try
			{
				httpForCellSelect.send(requestForCell);	
			}
			catch(err:Error)
			{
				//Alert.show("error in request to server for selecting a cell "+err.message);
			}
			//Release the memory
			if(requestForCell != null)
			{		
				delete requestForCell.UID;
				delete requestForCell.space;
				delete requestForCell.x;
				delete requestForCell.y;
				delete requestForCell.rand;
				
				requestForCell = null;
			}
		}
		public function onFault(evt:FaultEvent):void
		{
			//Alert.show("Error while requesting for data at  " + httpService.url.toString()+" in Replayer.as");
		}
		private function onFaultCellSelecting(evt:FaultEvent):void
		{
			//Alert.show("Error while requesting for selecting cell url " + httpForCellSelect.url.toString()+" in Replayer.as");
		}
		
		/**
		 * This method contain clean up of the memory 
		 */
		public function onDestroy():void
		{
			httpService.removeEventListener(ResultEvent.RESULT,onResult);
			httpForCellSelect.removeEventListener(ResultEvent.RESULT,onCellResult);
			myTimer.removeEventListener(TimerEvent.TIMER,timerHandler);
			httpService.removeEventListener(FaultEvent.FAULT,onFault);
			httpForCellSelect.removeEventListener(FaultEvent.FAULT,onFaultCellSelecting);
			
			if(request != null)
			{
				delete request.UID;
				delete request.space;
				delete request.rand;
			}
			space_Bounds		=	null;
			ghost_EntityData	=	null;
			entity_Data			=	null;
			cell_Boundary		=	null;
			myTimer				=	null;
			httpService			=	null;
			httpForCellSelect	=	null;
			UID					=	null;
			space				=	null;
			request				=	null;
			requestForCell		=	null;
			serviceURL			=	null;
			serviceURLForCell	=	null;
			cellRect			=	null;
			cellRects			=	null;
			cellDataIds			=	null;
			cellDataIps			=	null;
			cellDataLoads		=	null;
			typeNames			=	null;
			entityBounds		=	null;
			branchLoads			=	null;
			spaceGeometry 		= 	null;
			chunk_Bounds		=	null;
		}
	}
}