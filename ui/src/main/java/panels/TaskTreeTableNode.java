package panels;

import data.Task;
import data.TaskState;
import org.jdesktop.swingx.treetable.AbstractMutableTreeTableNode;
import org.jdesktop.swingx.treetable.TreeTableNode;

import javax.swing.*;
import java.util.Objects;

public class TaskTreeTableNode extends AbstractMutableTreeTableNode {
    private final TaskTreeTableNode parent;
    private Task task;

    public TaskTreeTableNode(TaskTreeTableNode parent, Task task) {
        this.parent = parent;
        this.task = task;
    }

    @Override
    public Object getValueAt(int i) {
        if (task == null) {
            return "?";
        }
        switch (i) {
            case 0:
                return task.name;
        }
        return "?";
    }

    @Override
    public int getColumnCount() {
        return 1; // just name for now
    }

    @Override
    public TreeTableNode getParent() {
        return parent;
    }

    @Override
    public boolean getAllowsChildren() {
        return true;
    }

    @Override
    public boolean isLeaf() {
        if (task == null) {
            return false;
        }
        return super.isLeaf();
    }

    @Override
    public boolean isEditable(int i) {
        return false;
    }

    @Override
    public void setValueAt(Object o, int i) {
    }

    @Override
    public Object getUserObject() {
        return task;
    }

    @Override
    public void setUserObject(Object o) {
        if (o instanceof Task) {
            task = (Task) o;
        }
    }

    public boolean isActiveTask() {
        if (task == null) {
            return false;
        }
        return task.state == TaskState.ACTIVE;
    }

    public boolean hasActiveChildTask() {
        if (children.isEmpty()) {
            return false;
        }
        for (int i = 0; i < getChildCount(); i++) {
            TaskTreeTableNode child = (TaskTreeTableNode) getChildAt(i);

            if (child.isActiveTask() || child.hasActiveChildTask()) {
                return true;
            }
        }
        return false;
    }
}
