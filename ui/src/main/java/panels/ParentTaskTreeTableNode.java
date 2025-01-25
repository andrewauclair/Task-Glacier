package panels;

import data.Task;
import org.jdesktop.swingx.treetable.TreeTableModel;
import org.jdesktop.swingx.treetable.TreeTableNode;

public class ParentTaskTreeTableNode extends TaskTreeTableNode {
    public ParentTaskTreeTableNode() {
        super(null, null);
    }

    public void addTask(Task task) {
        // find the parent task first, then add the node to that
        if (task.parentID == 0) {
            add(new TaskTreeTableNode(this, task));
        }
        else {
            TaskTreeTableNode node = findTaskNode(this, task.parentID);

            if (node != null) {
                node.add(new TaskTreeTableNode(node, task));
            }
        }
    }

    public void updateTask(Task task) {

    }

    private static TaskTreeTableNode findTaskNode(TaskTreeTableNode node, int taskID) {
        for (int i = 0; i < node.getChildCount(); i++) {
            TreeTableNode child = node.getChildAt(i);

            if (((Task) child.getUserObject()).id == taskID) {
                return (TaskTreeTableNode) child;
            }
            else {
                TaskTreeTableNode next = findTaskNode((TaskTreeTableNode) child, taskID);

                if (next != null) {
                    return next;
                }
            }
        }
        return null;
    }
}
