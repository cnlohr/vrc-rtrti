
using UnityEngine;
using VRC.SDK3.Components;
using VRC.SDKBase;
using VRC.Udon;
using UdonSharp;

/// <summary>
/// A Basic example class that demonstrates how to toggle a list of object on and off when someone interacts with the UdonBehaviour
/// This toggle only works locally
/// </summary>
[AddComponentMenu("Udon Sharp/Utilities/Mat And Interact Toggle")]
[UdonBehaviourSyncMode(BehaviourSyncMode.NoVariableSync)]
public class MatAndInteractToggle : UdonSharpBehaviour 
{
	[Tooltip("List of objects to toggle on and off")]
	public GameObject[] toggleObjects;
	public Material[] matFlip;

	public override void Interact()
	{
		bool bLastSet = false;
		foreach (GameObject toggleObject in toggleObjects)
		{
			toggleObject.SetActive(bLastSet = !toggleObject.activeSelf);
		}
		
		foreach (Material m in matFlip)
		{
			m.SetFloat( "_Flip", bLastSet?1.0f:0.0f );
		}
		
	}
}
