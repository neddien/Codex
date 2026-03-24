using CodexEditor.ViewModel;
using System;
using System.Linq;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Windows.Documents;
using CodexEditor.Util;
using System.Runtime.Serialization;

namespace CodexEditor.ViewModel.GameProject
{
	[DataContract]
	class ProjectData
	{
		[DataMember]
		public string ProjectName { get; set; }
		[DataMember]
		public string ProjectPath { get; set; }
		[DataMember]
		public DateTime Date { get; set; }
		public string FullPath { get => $"{ProjectPath}{ProjectName}{Project.Extension}"; }
		public byte[] Icon { get; set; }
		public byte[] PreviewImage { get; set; }
	}

	[DataContract]
	class ProjectDataList
	{
		[DataMember]
		public List<ProjectData> Projects { get; set; }
	}

	class OpenProject
	{
		private static readonly string _appdata = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "/Advres/CodexEditor/";
		private static readonly string _projectDataPath;
		private static readonly ObservableCollection<ProjectData> _projects = new ObservableCollection<ProjectData>();
		public static ReadOnlyObservableCollection<ProjectData> Projects { get; }

		static OpenProject()
		{
			try
			{
				if (!Directory.Exists(_appdata))
					Directory.CreateDirectory(_appdata);
				_projectDataPath = Path.Combine(_appdata, "ProjectData.xml");
				Projects = new ReadOnlyObservableCollection<ProjectData>(_projects);
				ReadProjectData();
			}
			catch (Exception ex)
			{
				Console.WriteLine(ex.Message);
				Logger.Log(MessageType.Error, $"Failed to open project: {ex.Message}");
				throw;
			}
		}

		private static void ReadProjectData()
		{
			if (File.Exists(_projectDataPath))
			{
				var projects = Serializer.FromFile<ProjectDataList>(_projectDataPath).Projects.OrderByDescending(x => x.Date);
				_projects.Clear();
				foreach (var project in projects)
				{
					if (File.Exists(project.FullPath))
					{
						project.Icon = File.ReadAllBytes($"{project.ProjectPath}/icon.png");
						project.PreviewImage = File.ReadAllBytes($"{project.ProjectPath}/preview_image.png");
						_projects.Add(project);
					}
				}
				WriteProjectData();
			}
		}

		private static void WriteProjectData()
		{
			var projects = _projects.OrderBy(x => x.Date).ToList();
			var projectDataList = new ProjectDataList();
			projectDataList.Projects = projects;
			Serializer.ToFile(_projectDataPath, projectDataList);
		}

		public static Project Open(ProjectData projectData)
		{
			ReadProjectData();
			var project = _projects.FirstOrDefault(x => x.FullPath == projectData.FullPath);
			if (project != null)
			{
				project.Date = DateTime.Now;
				WriteProjectData();
			}
			else
			{
				project = projectData;
				project.Date = DateTime.Now;
				_projects.Add(project);
				WriteProjectData();
			}
			return Project.LoadProject(project.FullPath);
		}
	}
}
