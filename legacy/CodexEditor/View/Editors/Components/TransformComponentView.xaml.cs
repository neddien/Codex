using CodexEditor.Util;
using CodexEditor.ViewModel.ECS;
using CodexEditor.ViewModel.GameProject;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace CodexEditor.View.Editors.Components
{
	/// <summary>
	/// Interaction logic for TransformComponentView.xaml
	/// </summary>
	public partial class TransformComponentView : UserControl
	{
		private Action _undoAction = null;
		private bool _propertyChanged = false;

		public TransformComponentView()
		{
			InitializeComponent();
		}

		private Action GetAction(Func<TransformComponent, (TransformComponent transform, Vector3)> selector,
			Action<(TransformComponent transform, Vector3)> forEachAction)
		{
			if (!(DataContext is MSTransformComponent vm))
			{
				_undoAction = null;
				_propertyChanged = false;
				return null;
			}
			var selection = vm.SelectedComponents.Select(x => selector(x)).ToList();
			return new Action(() =>
			{
				selection.ForEach(x => forEachAction(x));
				(EntityEditorView.Instance.DataContext as MSEntityModel)?.GetMSComponent<MSTransformComponent>().Refresh();
			});
		}

		private Action GetPositionAction() => GetAction((x) => (x, x.Position), (x) => x.transform.Position = x.Item2);
		private Action GetRotationAction() => GetAction((x) => (x, x.Rotation), (x) => x.transform.Rotation = x.Item2);
		private Action GetScaleAction() => GetAction((x) => (x, x.Scale), (x) => x.transform.Scale = x.Item2);

		private void CaptureActions(Action redoAction, string actionName)
		{
			if (_propertyChanged)
			{
				Debug.Assert(_undoAction != null);
				_propertyChanged = false;
				Project.Unredoable.Add(new UnredoableAction(_undoAction, redoAction, actionName));
			}
		}

		// Position undo
		private void positionVectorBox_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			_propertyChanged = false;
			_undoAction = GetPositionAction();
        }

		private void positionVectorBox_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			CaptureActions(GetPositionAction(), "Position changed");
        }

		private void positionVectorBox_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
		{
			if (_propertyChanged && _undoAction != null)
				positionVectorBox_PreviewMouseLeftButtonUp(sender, null);
        }

		// Rotation undo
		private void rotationVectorBox_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			_propertyChanged = false;
			_undoAction = GetRotationAction();
		}

		private void rotationVectorBox_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			CaptureActions(GetRotationAction(), "Rotation changed");
		}

		private void rotationVectorBox_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
		{
			if (_propertyChanged && _undoAction != null)
				rotationVectorBox_PreviewMouseLeftButtonUp(sender, null);
		}

		// Scale undo
		private void scaleVectorBox_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			_propertyChanged = false;
			_undoAction = GetScaleAction();
		}

		private void scaleVectorBox_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			CaptureActions(GetScaleAction(), "Scale changed");
		}

		private void scaleVectorBox_LostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
		{
			if (_propertyChanged && _undoAction != null)
				scaleVectorBox_PreviewMouseLeftButtonUp(sender, null);
		}

		private void UserControl_Loaded(object sender, RoutedEventArgs e)
		{
			// TODO: Have a Texture viewer for Texture2D's
			(DataContext as MSTransformComponent).PropertyChanged += (s, e) => _propertyChanged = true;
		}
	}
}
