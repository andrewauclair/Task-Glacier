package panels;

import data.Task;
import data.TaskModel;
import data.TaskState;

import javax.swing.*;
import java.awt.*;

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

    private void updateDisplay() {
        if (activeTask != null) {
            activeTaskDisplay.setText(activeTask.id + " - " + activeTask.name);
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
            updateDisplay();
        }
    }

    @Override
    public void updatedTask(Task task) {
        if (task == activeTask && task.state != TaskState.ACTIVE) {
            activeTask = null;
            updateDisplay();
        }
        else if (task.state == TaskState.ACTIVE) {
            activeTask = task;
            updateDisplay();
        }
    }
}
