package panels;

import data.Task;
import data.TaskModel;
import data.TaskState;

import javax.swing.*;
import java.awt.*;

import static taskglacier.MainFrame.mainFrame;

public class StatusBar extends JPanel implements TaskModel.Listener {
    private final JLabel activeTaskDisplay = new JLabel("No Active Task");
    private Task activeTask = null;

    public StatusBar() {
        super(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;

        add(activeTaskDisplay, gbc);
    }

    private void updateDisplay(TaskModel taskModel) {
        if (activeTask != null) {
            StringBuilder text = new StringBuilder(activeTask.id + " - ");

            int parentID = activeTask.parentID;

            while (parentID != 0) {
                Task task = taskModel.getTask(parentID);

                text.append(task.name);
                text.append(" / ");

                parentID = task.parentID;
            }

            activeTaskDisplay.setText(text.toString());
        }
        else {
            activeTaskDisplay.setText("No Active Task");
        }
    }

    @Override
    public void newTask(Task task) {
        // a new task could be the active task, if I decide to implement it that way
        if (task.state == TaskState.ACTIVE) {
            activeTask = task;
            updateDisplay(mainFrame.getTaskModel());
        }
    }

    @Override
    public void updatedTask(Task task) {
        if (task == activeTask && task.state != TaskState.ACTIVE) {
            activeTask = null;
            updateDisplay(mainFrame.getTaskModel());
        }
        else if (task.state == TaskState.ACTIVE) {
            activeTask = task;
            updateDisplay(mainFrame.getTaskModel());
        }
    }
}
