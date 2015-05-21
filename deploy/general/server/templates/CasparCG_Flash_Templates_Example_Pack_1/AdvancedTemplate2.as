/*
 * This template uses the SharedData module to be able to communicate and share data with other instances loaded in the template host. 
 * 
 * To compile this template you will need the free Tweener library found at http://code.google.com/p/tweener/
*/

package
{
	import caurina.transitions.Equations;
	import caurina.transitions.Tweener;
	import flash.display.MovieClip;
	import flash.events.TimerEvent;
	import flash.text.TextField;
	import flash.utils.Timer;
	import se.svt.caspar.ICommunicationManager;
	import se.svt.caspar.IRegisteredDataSharer;
	import se.svt.caspar.template.CasparTemplate;
	
	public class AdvancedTemplate2 extends CasparTemplate implements IRegisteredDataSharer
	{
		//The name of the id where the data is stored. Should be somewhat project specific if there are template from other projects playing that are using the SharedData module. In this case we prefix with "AT".
		private const DATA_POSITION_ID:String = "ATposition";
		//Some constants
		private const POSITION_UP:String = "up";
		private const POSITION_DOWN:String = "down";
		private const POSITION_IN_X:Number = 700;
		private const POSITION_OUT_X:Number = 1030;
		private const POSITION_UP_Y:Number = 100;
		private const POSITION_DOWN_Y:Number = 175;
		private const POSITION_OUT_Y:Number = 620;
		//The plate instance on the stage
		public var plate:MovieClip;
		//Some flags to check when the template should start
		private var _hasCommunicationManager:Boolean = false;
		private var _hasBeenPlayed:Boolean = false;
		//Holds the current position of the template defined by POSITION_UP and POSITION_DOWN
		private var _currentPostion:String;
		
		public function AdvancedTemplate2()
		{
			//set the initial x position for the plate
			plate.x = POSITION_OUT_X;
		}
		
		//overridden postInitialize that allows us to access the communication manager
		override public function postInitialize():void 
		{
			//set the flag
			_hasCommunicationManager = true;
			//if the template has already recieved a play command
			if (_hasBeenPlayed) {
				//write the position
				writePosition(POSITION_UP);
				//do the intro animation
				introAnimation();
			}
			
		}
		
		//overridden preDispose that will be called just before the template is removed by the template host. Allows us to clean up.
		override public function preDispose():void 
		{
			//dispose the tweener engine
			Tweener.removeAllTweens();
		}
		
		//overridden Stop that initiates the outro animation. IMPORTANT, it is very important when you override Stop to later call super.Stop() or removeTemplate()
		override public function Stop():void 
		{
			//do the outro animation
			outroAnimation();
		}
		
	
		
		//overridden Play that will initiate the intro animation and start the timer if there is data
		override public function Play():void 
		{
			//set the flag
			_hasBeenPlayed = true;
			//if the communication manager is available
			if (_hasCommunicationManager)
			{
				//write the postition
				writePosition(POSITION_UP);
				//do the intro animation
				introAnimation();
			}	
		}		
		
		//writes a position to the SharedData module
		private function writePosition(position:String):void
		{
			//we use an object as data to be able to specify borh position and sender
			var dataObject:Object = { position: position, sender: this.name }
			//write the data
			this.communicationManager.sharedData.writeData(this, DATA_POSITION_ID, dataObject);
			//store the current position internally
			_currentPostion = position;
		}
		
		//animate in from right to left
		private function introAnimation():void
		{
			Tweener.addTween(plate, { x: POSITION_IN_X, time: 1, delay: .2, transition: Equations.easeOutElastic } );
		}
		
		//animate out dowm and right
		private function outroAnimation():void
		{
			Tweener.addTween(plate, { y: POSITION_OUT_Y, x: POSITION_OUT_X, time: .6, transition: Equations.easeInSine, onComplete: removeTemplate } );
		}
		
		/* INTERFACE se.svt.caspar.IRegisteredDataSharer */
		
		//will be called by shared data when the id is changed
		public function onSharedDataChanged(id:String):void
		{
			//check that the id is the one we are interested in (in this case it always will be so since we only subscribe to one id). 
			//Also check that it is not this instance that changed the value
			if (id == DATA_POSITION_ID && this.communicationManager.sharedData.readData(id).sender != this.name)
			{
				//stores the occupied position
				var occupiedPosition:String = this.communicationManager.sharedData.readData(id).position;
		
				//check if it will interfere with our current position
				if (occupiedPosition == POSITION_UP && _currentPostion == POSITION_UP)
				{
					//move down this plate
					Tweener.addTween(plate, { y: POSITION_DOWN_Y, time: 1, delay: .2, transition: Equations.easeOutElastic } );
					//update our position
					writePosition(POSITION_DOWN);
				}
				if (occupiedPosition == POSITION_DOWN && _currentPostion == POSITION_DOWN)
				{
					//stop the template
					this.Stop();
				}
			}
		}
	}
}