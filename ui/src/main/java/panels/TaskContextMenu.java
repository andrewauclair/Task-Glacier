package panels;

import data.Task;
import dialogs.AddModifyTask;
import dialogs.TaskConfig;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.PacketType;
import packets.TaskStateChange;
import taskglacier.MainFrame;

import javax.swing.*;

public class TaskContextMenu extends JPopupMenu {
    private final MainFrame mainFrame;
    private final TaskDisplay display;

    public interface TaskDisplay {
        Task taskForSelectedRow();
    }
    JMenuItem add = new JMenuItem("Add Task...");
    JMenuItem addSubTask = new JMenuItem("Add Sub-Task...");
    JMenuItem start = new JMenuItem("Start");
    JMenuItem startStopActive = new JMenuItem("Start (Stop Active)");
    JMenuItem startFinishActive = new JMenuItem("Start (Finish Active)");
    JMenuItem stop = new JMenuItem("Stop");
    JMenuItem finish = new JMenuItem("Finish");
    JMenuItem openInNewWindow = new JMenuItem("Open in New List");
    JMenuItem config = new JMenuItem("Configure...");

    public TaskContextMenu(MainFrame mainFrame, TaskDisplay display) {
        this.mainFrame = mainFrame;
        this.display = display;

        config.addActionListener(e -> {
            Task task = display.taskForSelectedRow();

            if (task == null) {
                return;
            }

            TaskConfig dialog = new TaskConfig(mainFrame, task);
            dialog.setVisible(true);
        });

        start.addActionListener(e -> changeTaskState(PacketType.START_TASK));
        startStopActive.addActionListener(e -> changeTaskState(PacketType.START_TASK));
        startFinishActive.addActionListener(e -> {
            finishActiveTask();
            changeTaskState(PacketType.START_TASK);
        });
        stop.addActionListener(e -> changeTaskState(PacketType.STOP_TASK));
        finish.addActionListener(e -> changeTaskState(PacketType.FINISH_TASK));

        add.addActionListener(e -> new AddModifyTask(mainFrame, 0, false).setVisible(true));

        addSubTask.addActionListener(e -> {
            Task task = display.taskForSelectedRow();

            if (task == null) {
                return;
            }

            new AddModifyTask(mainFrame, task.id, false).setVisible(true);
        });

        openInNewWindow.addActionListener(e -> {
            Task task = display.taskForSelectedRow();

            if (task == null) {
                return;
            }

            if (!Docking.isDockableRegistered("task-list-" + task.id)) {
                TasksLists newList = new TasksLists(mainFrame, task);
                Docking.display(newList);
//                Docking.dock(newList, TasksLists.this, DockingRegion.CENTER);
            }
            else {
                Docking.display("task-list-" + task.id);
            }
        });
    }

    private void finishActiveTask() {
        TaskStateChange change = new TaskStateChange();
        change.packetType = PacketType.FINISH_TASK;
        change.taskID = mainFrame.getTaskModel().getActiveTaskID().get();
        mainFrame.getConnection().sendPacket(change);
    }

    private void changeTaskState(PacketType type) {
        Task task = display.taskForSelectedRow();

        if (task == null) {
            return;
        }

        TaskStateChange change = new TaskStateChange();
        change.packetType = type;
        change.taskID = task.id;
        mainFrame.getConnection().sendPacket(change);
    }
}
