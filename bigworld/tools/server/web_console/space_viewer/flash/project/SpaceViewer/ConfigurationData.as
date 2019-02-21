package
{
	import mx.controls.Alert;
	import flash.utils.Dictionary;
	import mx.collections.ArrayCollection;
	
	/**
	 * This class consists of the all the variables that match the configuration screen 
	 * values
	 */
	public class ConfigurationData
	{
		private var m_CellAppUpdateFrequency:Number			=	1.00;
		private var m_CellAppManagerUpdateFrequency:Number	=	1.00;
		private var m_CellAppIDColour:uint					=	0x000000;
		private var m_IPAddressColour:uint					=	0x000000;
		private var m_CellLoadColour:uint					=	0xff0000;
		private var m_PartitionLoadColour:uint				=	0x004080;
		private var m_EntityBoundsColour:uint				=	0x000000;
		private var m_SpaceBoundsColour:uint				=	0x590679;
		private var m_GridColour:uint						=	0x00ff00;
		private var m_GhostEntityColour:uint				=	0xc0c0c0;
		private var m_CellBoundaryColour:uint				=	0x0000ff;
		private var m_ImagePath:String;
		private var m_EntitySize:Number;
		private var m_IconPath:String;
		private var m_IsIconSelected:Boolean				=	false;
		private var m_IsImageOverlay:Boolean				=	false;
		private var m_IsPartitionLoadEnabled:Boolean		=	false;
		private var m_IsEntityBoundsEnabled:Boolean			=	false;
		private var m_IsRelativeColour:Boolean				=	false;
		
		public var iconarray:Array;
		
		public var imageOverlayArray:Array;	
		public var imageSpaceDict:Dictionary;
		public var currentIcons:Array;	
		
		// ------Setter and Getter methods related to View Tab in Configuartion Screen-----------------
		
		// Setting the cell app update frequency
		public function set CellAppUpdateFrequency(freq:Number):void
		{
			m_CellAppUpdateFrequency = freq;
		}
		
		// getting the cell app update freuency
		public function get CellAppUpdateFrequency():Number
		{
			return m_CellAppUpdateFrequency;
		}
		
		// setting the cell app manager update frequency
		public function set CellAppManagerUpdateFrequency(freq:Number):void
		{
			m_CellAppManagerUpdateFrequency = freq;
		}
		
		// getting the cell app manager update frequency
		public function get CellAppManagerUpdateFrequency():Number
		{
			return m_CellAppManagerUpdateFrequency;
		}
		
		// -------Setter and Getter methods related to Colour Tab in Configuration screen--------------------
		// setting the cell app id colour
		public function set CellAppIDColour(colour:uint):void
		{
			m_CellAppIDColour = colour;
		}
		
		// getting the cell app id colour
		public function get CellAppIDColour():uint
		{
			return m_CellAppIDColour;
		}
		
		// setting the IP address colour
		public function set IPAddressColour(colour:uint):void
		{
			m_IPAddressColour = colour;
		}
		
		// getting the IP address colour
		public function get IPAddressColour():uint
		{
			return m_IPAddressColour;
		}
		
		// setting the cell load colour 
		public function set CellLoadColour(colour:uint):void
		{
			m_CellLoadColour = colour;
		}
		
		// getting the cell load colour
		public function get CellLoadColour():uint
		{
			return m_CellLoadColour;
		}
		
		// setting the partition load colour 
		public function set PartitionLoadColour(colour:uint):void
		{
			m_PartitionLoadColour = colour;
		}
		
		// getting the partition load colour
		public function get PartitionLoadColour():uint
		{
			return m_PartitionLoadColour;
		}
		
		// setting the entity bounds colour 
		public function set EntityBoundsColour(colour:uint):void
		{
			m_EntityBoundsColour = colour;
		}
		
		// getting the entity bounds colour
		public function get EntityBoundsColour():uint
		{
			return m_EntityBoundsColour;
		}
		
		// setting the space bounds colour 
		public function set SpaceBoundsColour(colour:uint):void
		{
			m_SpaceBoundsColour = colour;
		}
		
		// getting the space bounds colour
		public function get SpaceBoundsColour():uint
		{
			return m_SpaceBoundsColour;
		}
		
		// setting the grid colour 
		public function set GridColour(colour:uint):void
		{
			m_GridColour = colour;
		}
		
		// getting the grid colour
		public function get GridColour():uint
		{
			return m_GridColour;
		}
		
		// setting the ghost entity colour 
		public function set GhostEntityColour(colour:uint):void
		{
			m_GhostEntityColour = colour;
		}
		
		// getting the ghost entity colour
		public function get GhostEntityColour():uint
		{
			return m_GhostEntityColour;
		}
		
		// setting the cell boundary colour 
		public function set CellBoundaryColour(colour:uint):void
		{
			m_CellBoundaryColour = colour;
		}
		
		// getting the cell boundary colour
		public function get CellBoundaryColour():uint
		{
			return m_CellBoundaryColour;
		}
		
		public function set IsRelativeColour(flag:Boolean):void
		{
			m_IsRelativeColour = flag;
		}
		
		public function get IsRelativeColour():Boolean
		{
			return m_IsRelativeColour;
		}
		public function set IsEntityBoundsEnabled(flag:Boolean):void
		{
			m_IsEntityBoundsEnabled = flag;
		}
		
		public function get IsEntityBoundsEnabled():Boolean
		{
			return m_IsEntityBoundsEnabled;
		}
		
		public function set IsPartitionLoadEnabled(flag:Boolean):void
		{
			m_IsPartitionLoadEnabled = flag;
		}
		
		public function get IsPartitionLoadEnabled():Boolean
		{
			return m_IsPartitionLoadEnabled;
		}
		
		// -------Setter and Getter methods related to Image Overlay Tab in Configuration Screen------------------
		public function set ImagePath(path:String):void
		{
			m_ImagePath = path;
		}
		
		public function get ImagePath():String
		{
			return m_ImagePath;
		}
		
		public function set IsImageOverlay(flag:Boolean):void
		{
			m_IsImageOverlay = flag;
		}
		
		public function get IsImageOverlay():Boolean
		{
			return m_IsImageOverlay;
		}
		
		// -------Setter and Getter methods related to Entity Size Tab in Global Settings--------------------
		
		public function set EntitySize(size:Number):void
		{
			m_EntitySize = size;
		}
		
		public function get EntitySize():Number
		{
			return m_EntitySize;
		}
		
		// -------Setter and Getter methods related to Entity Icon Tab in Global Settings--------------------
		
		public function set IsIconSelected(flag:Boolean):void
		{
			m_IsIconSelected = flag;
		}
		
		public function get IsIconSelected():Boolean
		{
			return m_IsIconSelected;
		}
		
		public function set IconPath(path:String):void
		{
			m_IconPath = path;
		}
		
		public function get IconPath():String
		{
			return m_IconPath;
		}
		// ---------------------------------------------------------------------------------------------------------
	}
}