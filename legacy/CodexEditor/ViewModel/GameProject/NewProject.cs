using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using CodexEditor.Util;

namespace CodexEditor.ViewModel.GameProject
{
	[DataContract]
	class ProjectTemplate
	{
		public string TemplatePath { get; set; }
		[DataMember]
		public string ProjectType { get; set; }
		[DataMember]
		public string ProjectFile { get; set; }
		[DataMember]
		public List<string> ProjectFolders { get; set; }
		public string IconFilePath { get; set; }
		public byte[] PreviewImage { get; set; }
		public byte[] Icon { get; set; }
		public string PreviewFilePath { get; set; }
		public string ProjectFilePath { get; set; }
	}

	class NewProject : ViewModelBase
	{
		private readonly string _templatePath = "ProjectTemplates/";

		private string _projectName = "New project";
		public string ProjectName
		{
			get => _projectName;
			set
			{
				if (_projectName != value) 
				{
					_projectName = value;
					ValidateProperties();
					OnPropertyChanged();
				}
			}
		}

		private string _projectPath = $"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\\Codex Projects";
		public string ProjectPath
		{
			get => _projectPath;
			set
			{
				if (_projectPath != value)
				{
					_projectPath = value;
					ValidateProperties();
					OnPropertyChanged();
				}
			}
		}

		private bool _isValid = false;
		public bool IsValid
		{
			get => _isValid;
			set
			{
				if (_isValid != value)
				{
					_isValid = value;
					OnPropertyChanged();
				}
			}
		}

		private string _errorMessage = string.Empty;
		public string ErrorMessage
		{
			get => _errorMessage;
			set
			{
				if (_errorMessage != value)
				{
					_errorMessage = value;
					OnPropertyChanged();
				}
			}
		}

		private ObservableCollection<ProjectTemplate> _templates = new ObservableCollection<ProjectTemplate>();

		public ObservableCollection<ProjectTemplate> ProjectTemplates { get => _templates; }

		public NewProject()
		{
			try
			{
				var files = Directory.GetFiles(_templatePath, "template.xml", SearchOption.AllDirectories);
				Debug.Assert(files.Any());
				foreach (var f in files)
				{
					var folder = Path.GetDirectoryName(f);
					var obj = Serializer.FromFile<ProjectTemplate>(f);
					obj.TemplatePath = Path.GetFullPath(Path.GetDirectoryName(f));
					obj.IconFilePath = Path.Combine(Path.GetFullPath(folder), "icon.png");
					obj.PreviewFilePath = Path.Combine(Path.GetFullPath(folder), "preview_image.png");
					obj.Icon = File.ReadAllBytes(obj.IconFilePath);
					obj.PreviewImage = File.ReadAllBytes(obj.PreviewFilePath);
					_templates.Add(obj);
				}
				ValidateProperties();
			}
			catch (Exception ex)
			{
				Console.WriteLine(ex);
				Logger.Log(MessageType.Error, $"Failed to create new project: {ex.Message}");
				throw;
			}
		}

		public Project CreateProject(ProjectTemplate template)
		{
			ValidateProperties();
			if (!IsValid) return null;

			if (!Path.EndsInDirectorySeparator(ProjectPath))
				ProjectPath += '\\';

			var path = $"{ProjectPath}{ProjectName}\\";
			try
			{
				// Copy our files over to the new project path
				if (!Directory.Exists(path))
					Directory.CreateDirectory(path);
				foreach (var dir in template.ProjectFolders)
					Directory.CreateDirectory(Path.Combine(path, dir));
				var files = Directory.GetFiles(template.TemplatePath);
				foreach (var file in files)
					if (!file.EndsWith(".xml"))
						File.Copy(file, path + Path.GetFileName(file));

				var proj = new Project(ProjectName, path);
				Serializer.ToFile(path + ProjectName + Project.Extension, proj);
				ValidateProperties();

				return proj;
			}
			catch (Exception ex)
			{
				Console.WriteLine(ex);
				Logger.Log(MessageType.Error, $"Failed to create project: {ex.Message}");
				throw;
			}
		}

		private bool ValidateProperties()
		{
			var path = ProjectPath;
			if (!Path.EndsInDirectorySeparator(path)) path += '\\';
			path += $"{ProjectName}\\";

			IsValid = false;
			if (string.IsNullOrEmpty(ProjectName.Trim()))
				ErrorMessage = "Type in a valid project name.";
			else if (ProjectName.IndexOfAny(Path.GetInvalidFileNameChars()) != -1)
				ErrorMessage = "Invalid character(s) in project name.";
			else if (string.IsNullOrEmpty(ProjectPath.Trim()))
				ErrorMessage = "Type in a valid project path.";
			else if (ProjectPath.IndexOfAny(Path.GetInvalidPathChars()) != -1)
				ErrorMessage = "Invalid character(s) in project path.";
			else if (Directory.Exists(path) && Directory.EnumerateFileSystemEntries(path).Any())
				ErrorMessage = "Project path is not empty.";
			else
			{
				ErrorMessage = string.Empty;
				IsValid = true;
			}

			return IsValid;
		}
	}
}
