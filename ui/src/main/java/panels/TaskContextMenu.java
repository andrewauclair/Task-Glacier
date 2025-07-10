package panels;

import data.Task;
import data.TaskState;
import dialogs.AddModifyTask;
import dialogs.TaskConfig;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.PacketType;
import packets.TaskStateChange;
import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;

public class TaskContextMenu extends JPopupMenu {
    public interface TaskDisplay {
        Task taskForSelectedRow();
    }

    private final MainFrame mainFrame;
    private final Window parent;
    private final TaskDisplay display;

    private final JMenuItem add = new JMenuItem("Add Task...");
    private final JMenuItem addSubTask = new JMenuItem("Add Sub-Task...");
    private final JMenuItem start = new JMenuItem("Start");
    private final JMenuItem startStopActive = new JMenuItem("Start (Stop Active)");
    private final JMenuItem startFinishActive = new JMenuItem("Start (Finish Active)");
    private final JMenuItem stop = new JMenuItem("Stop");
    private final JMenuItem finish = new JMenuItem("Finish");
    private final JMenuItem openInNewWindow = new JMenuItem("Open in New List");
    private final JMenuItem config = new JMenuItem("Configure...");

    public TaskContextMenu(MainFrame mainFrame, Window parent, TaskDisplay display) {
        this.mainFrame = mainFrame;
        this.parent = parent;
        this.display = display;

        config.addActionListener(e -> {
            Task task = display.taskForSelectedRow();

            if (task == null) {
                return;
            }

            TaskConfig dialog = new TaskConfig(mainFrame, parent, task);
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

        add.addActionListener(e -> new AddModifyTask(mainFrame, parent, 0, false).setVisible(true));

        addSubTask.addActionListener(e -> {
            Task task = display.taskForSelectedRow();

            if (task == null) {
                return;
            }

            new AddModifyTask(mainFrame, parent, task.id, false).setVisible(true);
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

    @Override
    public void show(Component invoker, int x, int y) {
        add(config);

        if (mainFrame.getTaskModel().getActiveTaskID().isPresent() &&
                !mainFrame.getTaskModel().taskHasNonFinishedChildren(mainFrame.getTaskModel().getActiveTaskID().get()) &&
                !mainFrame.getTaskModel().getTask(mainFrame.getTaskModel().getActiveTaskID().get()).locked) {
            add(startStopActive);
            add(startFinishActive);
        }
        else {
            add(start);
        }

        add(stop);
        add(finish);
        addSeparator();
        add(addSubTask);

        Task task = display.taskForSelectedRow();

        start.setEnabled(task.state == TaskState.PENDING);
        startStopActive.setEnabled(task.state == TaskState.PENDING);
        startFinishActive.setEnabled(task.state == TaskState.PENDING);
        stop.setEnabled(task.state == TaskState.ACTIVE);
        finish.setEnabled(task.state != TaskState.FINISHED && !mainFrame.getTaskModel().taskHasNonFinishedChildren(task.id) && !task.locked);

        super.show(invoker, x, y);
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

    public void openConfigDialog() {
        config.doClick();
    }
}
