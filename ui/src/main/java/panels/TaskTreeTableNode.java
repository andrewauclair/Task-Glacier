package panels;

import data.Task;
import org.jdesktop.swingx.treetable.AbstractMutableTreeTableNode;
import org.jdesktop.swingx.treetable.TreeTableNode;

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
            return null;
        }
        switch (i) {
            case 0:
                return task.id;
            case 1:
                return task.name;
        }
        return null;
    }

    @Override
    public int getColumnCount() {
        return 2; // id and name for now
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
}
