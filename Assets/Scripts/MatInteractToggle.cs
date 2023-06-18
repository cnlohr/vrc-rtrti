
using UnityEngine;
using VRC.SDK3.Components;
using VRC.SDKBase;
using VRC.Udon;
using UdonSharp;

/// <summary>
/// A Basic example class that demonstrates how to toggle a list of object on and off when someone interacts with the UdonBehaviour
/// This toggle only works locally
/// </summary>
[AddComponentMenu("Udon Sharp/Utilities/Mat Interact Toggle")]
[UdonBehaviourSyncMode(BehaviourSyncMode.NoVariableSync)]
public class MatInteractToggle : UdonSharpBehaviour 
{
	[Tooltip("List of objects to toggle on and off")]
	public Material[] matFlip;
	public string matFlipName;
	public bool bOn;

	void Start()
	{
		foreach (Material m in matFlip)
		{
			m.SetFloat( matFlipName, bOn?1.0f:0.0f );
		}
	}

	public override void Interact()
	{
		bOn = !bOn;
		foreach (Material m in matFlip)
		{
			m.SetFloat( matFlipName, bOn?1.0f:0.0f );
		}
	}
	
	void Update()
	{
	}
}
