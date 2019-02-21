package
{
	import flash.events.MouseEvent;
	/**
	 * This class is used to select a particular area to be zoomed to whole view
	 */
	public class RubberBander
	{
		public var m_stpoint_x:int;
		public var m_stpoint_y:int;
		public var m_endpoint_x:int;
		public var m_endpoint_y:int;
		public var m_savepoint_x:int;
		public var m_savepoint_y:int;
		public var ShiftLMouseDown:Boolean;
		public var selected:Boolean;
		public var w:int;
		public var h:int;
		
		//Manages mouse events / rubberbanding 
		public function RubberBander():void
		{
			m_stpoint_x 	= 0;
			m_stpoint_y 	= 0;
			m_endpoint_x 	= 0;
			m_endpoint_y 	= 0;
			m_savepoint_x 	= 0;
			m_savepoint_y 	= 0;
			
			ShiftLMouseDown	= false;
			selected 		= false;
		}
		
		//Mouse Event handler for Rubberbanding purpose
		public function onMouseEvent(event:MouseEvent):void
		{
			if(event.shiftKey && event.type == MouseEvent.MOUSE_DOWN)
			{
				m_stpoint_x 	= event.localX;
				m_stpoint_y 	= event.localY;
				m_savepoint_x 	= m_stpoint_x;
				m_savepoint_y 	= m_stpoint_y;
				w 				= 0;
				h 				= 0;
				selected 		= false;
				ShiftLMouseDown = true;
			}
			else if(event.type == MouseEvent.MOUSE_MOVE)
			{
				if(ShiftLMouseDown)
				{
					m_endpoint_x 	= event.localX;
					m_endpoint_y 	= event.localY;
					w 				= m_endpoint_x - m_stpoint_x;
					h 				= m_endpoint_y - m_stpoint_y;
					m_savepoint_x 	= m_endpoint_x;
					m_savepoint_y 	= m_endpoint_y;
				}
			}
			else if(!(event.shiftKey && event.type == MouseEvent.MOUSE_DOWN))
			{
				selected 		= true;
				ShiftLMouseDown = false;
			} 
		}
		
		/**
		 * Function for getting the current selection by the user for Rubberbanding
		 */
		public function GetCurrentSelection():Array
		{
			var left_x:int 	= 0;
			var left_y:int 	= 0;
			var right_x:int = 0;
			var right_y:int = 0;
			var rect_dimension:Array;
			
			if(m_endpoint_y > m_stpoint_y)
			{
				right_x = m_endpoint_x;
				right_y = m_endpoint_y;
				left_x 	= m_stpoint_x;
				left_y 	= m_stpoint_y;
			}
			else if(m_endpoint_y < m_stpoint_y)
			{
				right_x = m_stpoint_x;
				right_y = m_stpoint_y;
				left_x 	= m_endpoint_x;
				left_y 	= m_endpoint_y; 
			}
			else
			{
				right_x = m_stpoint_x;
				right_y = m_stpoint_y;
				left_x 	= m_stpoint_x;
				left_y 	= m_stpoint_y;
			}
			rect_dimension = new Array(left_x,left_y,right_x,right_y);
			return rect_dimension;
		}
	}
}